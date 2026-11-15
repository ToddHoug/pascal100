#include "boards/compupro-disk2.h"

#include "boards/compupro-selch.h"
#include "core/bus.h"
#include "core/statefile.h"
#include "core/value.h"
#include "host/media.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>

namespace altair {

namespace {

// A DMA byte is one memory cycle the Selector Channel drives: three bus states, no waits.
constexpr uint32_t kDmaCycle = 3;

// CTL (write), p. 5.
constexpr uint8_t kCtlAttnN    = 0x80;  // 1 = release ATTN (STATUS transparent)
constexpr uint8_t kCtlRun      = 0x40;  // 1 = start the command in OP2-OP0
constexpr uint8_t kCtlOp2      = 0x20;  // also the step direction: 1 = in (D2_SIN)
constexpr uint8_t kCtlFaultClr = 0x04;

// STATUS (read), p. 5. Bits 3-0 are the selected drives' lines, active low.
constexpr uint8_t kStAttnN   = 0x80;
constexpr uint8_t kStTimeOut = 0x40;
constexpr uint8_t kStCrc     = 0x20;
constexpr uint8_t kStOverRun = 0x10;
constexpr uint8_t kStReadyN  = 0x08;
constexpr uint8_t kStSeekN   = 0x04;

// A seek's length. The M2302B buffers step pulses ("slave step mode", CE manual 4.x STEP)
// and seeks after the last one; SEEK COMPLETE is false until the heads are on the track.
// 30 ms is the manual's minimum positioning time (spec table: 30 / 70 / 140 ms).
constexpr uint64_t kSeekMs = 30;
constexpr uint8_t kStFaultN  = 0x02;
constexpr uint8_t kStTrack0N = 0x01;

// The commands, p. 8-10 (the manual numbers them 00-04 and prints no table).
enum Op : uint8_t { kNull = 0, kReadData = 1, kWriteData = 2, kWriteHeader = 3, kReadHeader = 4 };

constexpr uint8_t kFormatFill = 0xE5;  // the DISK2 utility's `data` pass (p. 3)

// The drives the DISK2 utility formats that CompuPro's systems use (p. 3), from the Fujitsu
// M2301B/M2302B CE manual's specification table: 244 cylinders, 4 or 8 heads.
struct Model {
    const char* name;
    int         cylinders;
    int         heads;
};
constexpr Model kModels[] = {{"m20", 244, 8}, {"m10", 244, 4}};

const Model* findModel(const std::string& name) {
    for (const Model& m : kModels)
        if (lowerAscii(name) == m.name) return &m;
    return nullptr;
}

int driveIndex(const std::string& unit) {
    if (unit.size() != 6 || unit.rfind("drive", 0) != 0) return -1;
    const char ch = unit[5];
    return (ch >= '0' && ch < '0' + CompuProDisk2Board::kDrives) ? ch - '0' : -1;
}

} // namespace

CompuProDisk2Board::CompuProDisk2Board() { mover_.d = this; }

// ---------------------------------------------------------------------------
// The bus. U51 compares A1-A7 with SW2; A0 picks the port (p. 11).
// ---------------------------------------------------------------------------
bool CompuProDisk2Board::decodes(const BusCycle& c) const {
    if (!enabled_) return false;
    if (c.type != Cycle::IoRead && c.type != Cycle::IoWrite) return false;
    const uint8_t p = c.port();
    if ((p & 0xFE) != port_) return false;
    // BASE+1 IN pulses STEP* and drives no data (p. 11): snoop() takes it.
    return c.type == Cycle::IoWrite || p == port_;
}

uint8_t CompuProDisk2Board::read(const BusCycle&) {
    return attn_ ? latched_ : liveStatus();
}

void CompuProDisk2Board::write(const BusCycle& c) {
    if (c.type != Cycle::IoWrite) return;
    if (c.port() == port_) writeCtl(c.data);
    else                   writeData(c.data);
}

void CompuProDisk2Board::snoop(const BusCycle& c) {
    if (enabled_ && c.type == Cycle::IoRead && c.port() == (uint8_t)(port_ | 1)) step();
}

bool CompuProDisk2Board::assertsInt() const {
    return irq_ == IrqJumper::Int && attn_;
}

uint8_t CompuProDisk2Board::assertsVi() const {
    return attn_ ? viBit(irq_) : 0;
}

// ---------------------------------------------------------------------------
// The registers.
// ---------------------------------------------------------------------------
void CompuProDisk2Board::writeCtl(uint8_t v) {
    ctl_ = v;
    // FAULT-CLR* reaches the drives the DRIVE register selects (p. 13).
    if (v & kCtlFaultClr)
        for (int i = 0; i < kDrives; ++i)
            if (driveReg_ & (0x10 << i)) drive_[(size_t)i].fault = false;
    if ((v & kCtlAttnN) && attn_) {
        attn_ = false;
        intChanged();
    }
    if ((v & kCtlRun) && !busy_) start();
}

void CompuProDisk2Board::writeData(uint8_t v) {
    switch ((ctl_ >> 3) & 0x03) {  // OP1/OP0 (p. 6)
        case 0: driveReg_ = v; break;
        case 1: cylReg_ = v; break;
        case 2: headReg_ = v; break;
        default: secReg_ = v; break;
    }
}

// One STEP pulse to every selected drive with a disk, OP2 = 1 inward (p. 6, D2_SIN).
void CompuProDisk2Board::step() {
    const bool in = (ctl_ & kCtlOp2) != 0;
    for (int i = 0; i < kDrives; ++i) {
        Drive& d = drive_[(size_t)i];
        if (!(driveReg_ & (0x10 << i)) || !d.img) continue;
        if (in && d.pos < d.img->tracks() - 1) ++d.pos;
        if (!in && d.pos > 0) --d.pos;
        // Every pulse restarts the seek: it ends 30 ms after the last one.
        if (clock_) d.seekEnd = clock_->now() + clock_->tStatesPer(1000) * kSeekMs;
    }
}

// The selected drives drive the status lines, open collector: a line is low (true) when any
// selected drive pulls it. With no drive selected, all four float high.
uint8_t CompuProDisk2Board::liveStatus() const {
    bool ready = false, fault = false, track0 = false, onTrack = false;
    const uint64_t now = clock_ ? clock_->now() : 0;
    for (int i = 0; i < kDrives; ++i) {
        const Drive& d = drive_[(size_t)i];
        if (!(driveReg_ & (0x10 << i)) || !d.img) continue;
        ready = true;
        fault = fault || d.fault;
        track0 = track0 || d.pos == 0;
        onTrack = onTrack || now >= d.seekEnd;
    }
    uint8_t s = 0;
    if (!attn_) s |= kStAttnN;
    if (timeout_) s |= kStTimeOut;
    if (crc_) s |= kStCrc;
    if (overrun_) s |= kStOverRun;
    if (!ready) s |= kStReadyN;
    if (!onTrack) s |= kStSeekN;  // seeking, or no drive: the line floats high
    if (!fault) s |= kStFaultN;
    if (!track0) s |= kStTrack0N;
    return s;
}

int CompuProDisk2Board::selectedDrive() const {
    int found = -1;
    for (int i = 0; i < kDrives; ++i)
        if (driveReg_ & (0x10 << i)) {
            if (found >= 0) return -2;
            found = i;
        }
    return found;
}

bool CompuProDisk2Board::headerMatches(const Drive& d) const {
    const int head = driveReg_ & 0x0F;
    return head < d.img->heads() && headReg_ == head && cylReg_ == (uint8_t)d.pos &&
           secReg_ < d.sectors;
}

// ---------------------------------------------------------------------------
// The state machine. RUN starts the command in OP2-OP0 on the drive US1/US0 names.
// ---------------------------------------------------------------------------
void CompuProDisk2Board::start() {
    busy_    = true;
    timeout_ = crc_ = overrun_ = false;
    op_      = (uint8_t)((ctl_ >> 3) & 0x07);
    unit_    = ctl_ & 0x03;
    Drive& d = drive_[(size_t)unit_];

    if (!d.img) {
        // The machine is clocked by the drive's own data clock (p. 16): no drive, no clock.
        hung_ = true;
        log_.push_back("command " + std::to_string(op_) + " on drive " + std::to_string(unit_) +
                       ", which has no disk: the state machine runs on the drive's clock, so "
                       "it waits for ever (a bus RESET stops it)");
        return;
    }
    if (selectedDrive() != unit_) {
        log_.push_back("command on drive " + std::to_string(unit_) +
                       " (US1/US0), but the DRIVE register selects another: TIME OUT");
        timeOut();
        return;
    }
    if (d.fault) {
        timeOut();  // a drive with WRITE FAULT takes no command until FAULT CLR (p. 10)
        return;
    }

    const int head = driveReg_ & 0x0F;
    xfer_.buf.clear();
    xfer_.done = 0;
    switch (op_) {
        case kNull: complete(); return;
        case kReadData:
        case kWriteData: {
            if (!headerMatches(d)) {
                timeOut();
                return;
            }
            if (op_ == kWriteData && d.img->readOnly()) {
                d.fault = true;
                log_.push_back("drive " + std::to_string(unit_) +
                               " is read-only: WRITE FAULT (FAULT CLR clears it)");
                complete();
                return;
            }
            xfer_.buf.assign((size_t)sectorSize_, 0);
            if (op_ == kReadData) {
                size_t n = xfer_.buf.size();
                if (!d.img->readSector(d.pos, head, secReg_, xfer_.buf.data(), &n)) {
                    timeOut();
                    return;
                }
            }
            xfer_.toMemory = op_ == kReadData;
            break;
        }
        case kWriteHeader:
            xfer_.buf.assign(3, 0);
            xfer_.toMemory = false;
            break;
        case kReadHeader:
            xfer_.buf = {(uint8_t)d.pos, (uint8_t)head, 0};
            xfer_.toMemory = true;
            break;
        default:
            log_.push_back("operation code " + std::to_string(op_) +
                           " is not one the manual lists (0-4): TIME OUT");
            timeOut();
            return;
    }
    xfer_.active = true;
    holdChanged();
}

StepResult CompuProDisk2Board::transferOne(Bus& bus) {
    if (!xfer_.active) return {0, RunStatus::Ok};
    SelectorChannelBoard* sc = channel(bus);
    if (!sc) {
        // Nobody generates the cycles: the bytes go nowhere (p. 9). The command ends; a write
        // leaves the disk as it was.
        overrun_ = true;
        xfer_.active = false;
        holdChanged();
        complete();
        return {0, RunStatus::Ok};
    }
    uint8_t& b = xfer_.buf[xfer_.done];
    if (xfer_.toMemory) sc->cycle(bus, b);
    else                b = sc->cycle(bus, SelectorChannelBoard::kFloating);
    if (++xfer_.done == xfer_.buf.size()) finishTransfer();
    return {kDmaCycle, RunStatus::Ok};
}

SelectorChannelBoard* CompuProDisk2Board::channel(Bus& bus) const {
    for (Board* b : bus.boards())
        if (auto* sc = dynamic_cast<SelectorChannelBoard*>(b); sc && sc->serves(priority_))
            return sc;
    return nullptr;
}

void CompuProDisk2Board::finishTransfer() {
    xfer_.active = false;
    holdChanged();
    Drive&    d    = drive_[(size_t)unit_];
    const int head = driveReg_ & 0x0F;
    if (op_ == kWriteData) {
        size_t n = xfer_.buf.size();
        if (!d.img->writeSector(d.pos, head, secReg_, xfer_.buf.data(), &n)) {
            log_.push_back("drive " + std::to_string(unit_) + ": the host write failed");
            timeOut();
            return;
        }
        // Every sector reaches the host file when it is written, as on every disk board here
        // (host/cardimg.cpp): a crash or a killed process loses nothing that was acknowledged.
        d.img->sync();
    } else if (op_ == kWriteHeader) {
        const uint8_t* h = xfer_.buf.data();
        if (h[0] != (uint8_t)d.pos || h[1] != head || h[2] >= d.sectors) {
            char m[200];
            std::snprintf(m, sizeof m,
                          "drive %d cylinder %d head %d: header %02X %02X %02X is not the "
                          "standard (cylinder, head, sector 0-%d) and is not kept",
                          unit_, d.pos, head, h[0], h[1], h[2], d.sectors - 1);
            log_.push_back(m);
        }
    }
    complete();
}

void CompuProDisk2Board::timeOut() {
    timeout_ = true;
    complete();
}

// The command is over: STATUS latches, ATTN comes up (and with it the interrupt).
void CompuProDisk2Board::complete() {
    busy_    = false;
    attn_    = true;
    latched_ = liveStatus();
    intChanged();
}

// ---------------------------------------------------------------------------
// RESET* stops the state machine and leaves ATTN asserted (p. 3). The registers keep their
// contents (the manual does not say), and the drives keep their heads where they are.
// ---------------------------------------------------------------------------
void CompuProDisk2Board::reset(Reset r) {
    busy_ = hung_ = false;
    timeout_ = crc_ = overrun_ = false;
    if (xfer_.active) {
        xfer_.active = false;
        holdChanged();
    }
    if (r == Reset::PowerOn) {
        ctl_ = driveReg_ = cylReg_ = headReg_ = secReg_ = 0;
        // A Fujitsu drive returns its heads to track 0 at power on (CE manual "RETURN TO ZERO").
        for (Drive& d : drive_) {
            d.pos     = 0;
            d.fault   = false;
            d.seekEnd = 0;
        }
    }
    attn_    = true;
    latched_ = liveStatus();
    intChanged();
}

void CompuProDisk2Board::power() { reset(Reset::PowerOn); }

// ---------------------------------------------------------------------------
// Properties: the switches and the interrupt jumper.
// ---------------------------------------------------------------------------
std::vector<Property> CompuProDisk2Board::properties() {
    std::vector<Property> p;
    {
        Property x;
        x.name  = "port";
        x.help  = "Base address (SW2-1..7 = A7..A1). Two ports: BASE = CTL / STATUS, BASE+1 = "
                  "data registers / STEP (default C8, CompuPro's)";
        x.kind  = Kind::Int;
        x.radix = 16;
        x.min   = 0;
        x.max   = 0xFE;
        x.get   = [this] { return Value::ofInt(port_); };
        x.set   = [this](const Value& v, std::string& err) {
            if (v.i() & 1) {
                err = "the base is even: the switches set A7-A1 only";
                return false;
            }
            port_ = (uint8_t)v.i();
            decodeChanged();
            return true;
        };
        p.push_back(std::move(x));
    }
    {
        Property x;
        x.name  = "dma_priority";
        x.help  = "DMA priority (SW1-2..5): the TMA bus-arbitration priority, 0-15; the higher "
                  "one gets the bus first. Default 10, CompuPro's. The Selector Channel's MODE "
                  "byte must name it. Two bus masters must not share a priority";
        x.kind  = Kind::Int;
        x.radix = 10;
        x.min   = 0;
        x.max   = 15;
        x.get   = [this] { return Value::ofInt(priority_); };
        x.set   = [this](const Value& v, std::string&) {
            priority_ = (int)v.i();
            return true;
        };
        p.push_back(std::move(x));
    }
    {
        Property x;
        x.name    = "sector_size";
        x.help    = "Sector size in bytes (SW1-6..10, one switch on). Default 1024, CompuPro's. "
                    "The disk images are laid out by it, so change it with no disk mounted";
        x.kind    = Kind::Enum;
        x.choices = {"128", "256", "512", "1024", "2048"};
        x.get     = [this] { return Value::ofStr(std::to_string(sectorSize_)); };
        x.set     = [this](const Value& v, std::string& err) {
            for (const Drive& d : drive_)
                if (d.img) {
                    err = "unmount the disks first: an image is laid out by the sector size";
                    return false;
                }
            sectorSize_ = std::stoi(v.s());
            return true;
        };
        p.push_back(std::move(x));
    }
    p.push_back(irqJumperProperty(
        "interrupt",
        "Where ATTN (command done) goes: J7-J14 = vi0-vi7, J15 = int, or none (default). "
        "CompuPro's MP/M 8-16 uses vi1 (J8)",
        irq_));
    return p;
}

std::vector<MapEntry> CompuProDisk2Board::ioMap() const {
    return {
        {(uint32_t)port_, (uint32_t)port_, "read/write", "CTL (out) / STATUS (in)"},
        {(uint32_t)port_ + 1, (uint32_t)port_ + 1, "read/write",
         "DRIVE, CYLINDER, HEAD or SECTOR by CTL OP1/OP0 (out); an IN is one STEP pulse, no data"},
    };
}

// ---------------------------------------------------------------------------
// Units, MOUNT, UNMOUNT, and [[board.drive]].
// ---------------------------------------------------------------------------
std::vector<UnitDef> CompuProDisk2Board::units() const {
    std::vector<UnitDef> u;
    for (int i = 0; i < kDrives; ++i) {
        const Drive& d = drive_[(size_t)i];
        UnitDef x;
        x.name  = "drive" + std::to_string(i);
        x.kind  = UnitKind::Disk;
        x.state = d.img ? d.path : "(empty)";
        if (d.img) {
            x.readOnly       = d.img->readOnly();
            x.readOnlyForced = d.img->readOnlyForced();
        }
        u.push_back(std::move(x));
    }
    return u;
}

// The image is the drive's whole formatted capacity, cylinder by cylinder, head by head,
// sectors from 0. An empty file is a new disk: it is filled to that size with E5, as the
// DISK2 utility's `data` pass leaves a drive.
bool CompuProDisk2Board::mount(const std::string& unit, const std::string& path, bool ro,
                               std::string& err) {
    const int i = driveIndex(unit);
    if (i < 0) {
        err = "no unit `" + unit + "` on " + id + " (it has drive0..drive3)";
        return false;
    }
    Drive&       d = drive_[(size_t)i];
    const Model* m = findModel(d.model);
    if (!m) {
        err = "unknown drive type `" + d.model + "`";
        return false;
    }
    auto media = openMedia(resolvePath(path), ro, err);
    if (!media) {
        err += pathNote(path);
        return false;
    }
    auto img = std::make_unique<DiskImage>(std::move(media));
    img->init(m->cylinders, m->heads, true);
    img->initFormat(0, m->cylinders - 1, 0, m->heads - 1, Density::DD, d.sectors, sectorSize_, 0);
    const uint64_t want = (uint64_t)m->cylinders * (uint64_t)m->heads * (uint64_t)d.sectors *
                          (uint64_t)sectorSize_;
    const uint64_t have = img->size();
    if (have == 0 && !img->readOnly()) {
        img->setExtendsOnWrite(true);
        std::vector<uint8_t> fill((size_t)sectorSize_, kFormatFill);
        for (int c = 0; c < m->cylinders; ++c)
            for (int h = 0; h < m->heads; ++h)
                for (int s = 0; s < d.sectors; ++s) {
                    size_t n = fill.size();
                    if (!img->writeSector(c, h, s, fill.data(), &n)) {
                        err = "cannot write the new disk " + path;
                        return false;
                    }
                }
        img->setExtendsOnWrite(false);
        img->sync();
        log_.push_back(unit + ": new disk, " + std::to_string(want) + " bytes of E5 (" + m->name +
                       ")");
    } else if (!sizeMatches(have, want)) {
        err = std::to_string(have) + " bytes is not a " + m->name + " disk: " +
              std::to_string(m->cylinders) + " cylinders x " + std::to_string(m->heads) +
              " heads x " + std::to_string(d.sectors) + " sectors x " +
              std::to_string(sectorSize_) + " bytes = " + std::to_string(want) +
              ". An empty file becomes a new disk";
        return false;
    }
    d.img  = std::move(img);
    d.path = path;
    d.pos  = std::min(d.pos, m->cylinders - 1);
    if (d.img->readOnlyForced())
        log_.push_back(unit + " mounted WRITE-PROTECTED -- the host will not let us write " + path);
    return true;
}

bool CompuProDisk2Board::unmount(const std::string& unit, std::string& err) {
    const int i = driveIndex(unit);
    if (i < 0) {
        err = "no unit `" + unit + "` on " + id;
        return false;
    }
    Drive& d = drive_[(size_t)i];
    if (!d.img) {
        err = id + ":" + unit + " is empty";
        return false;
    }
    d.img->sync();
    d.img.reset();
    d.path.clear();
    return true;
}

Property CompuProDisk2Board::typeKey() const {
    Property x;
    x.name = "type";
    x.help = "The drive: m20 (Fujitsu M2302B, 8 heads, default) or m10 (M2301B, 4 heads); "
             "244 cylinders";
    x.kind = Kind::Enum;
    for (const Model& m : kModels) x.choices.push_back(m.name);
    return x;
}

Property CompuProDisk2Board::sectorsKey() const {
    Property x;
    x.name  = "sectors";
    x.help  = "Sectors per track, set by the drive's own sector switches. Default 11 (1090-byte "
              "slots, the setting for 1024-byte sectors)";
    x.kind  = Kind::Int;
    x.radix = 10;
    x.min   = 1;
    x.max   = 256;
    return x;
}

std::vector<Property> CompuProDisk2Board::mountKeys(const std::string& unit) const {
    if (driveIndex(unit) < 0) return {};
    return {typeKey(), sectorsKey()};
}

// The type and sectors go with the disk: a mount that is refused leaves the old ones.
bool CompuProDisk2Board::mountWith(const std::string& unit, const std::string& path, bool ro,
                                   const KeyValues& keys, std::string& err) {
    const int i = driveIndex(unit);
    if (i < 0) return mount(unit, path, ro, err);
    Drive&            d       = drive_[(size_t)i];
    const std::string model   = d.model;
    const int         sectors = d.sectors;
    for (const auto& [k, v] : keys) {
        if (k == "type" && !v.empty()) d.model = lowerAscii(v);
        else if (k == "sectors" && !v.empty()) d.sectors = std::stoi(v);
    }
    if (mount(unit, path, ro, err)) return true;
    d.model   = model;
    d.sectors = sectors;
    return false;
}

std::vector<Property> CompuProDisk2Board::subUnitProperties(const std::string& table) const {
    if (table != "drive") return {};
    std::vector<Property> p;
    {
        Property x;
        x.name  = "unit";
        x.help  = "Which drive (0..3): the J1-J4 data cable and DRIVE register bit 4+n";
        x.kind  = Kind::Int;
        x.radix = 10;
        x.min   = 0;
        x.max   = kDrives - 1;
        p.push_back(std::move(x));
    }
    {
        Property x;
        x.name = "mount";
        x.help = "The disk image to put in it. Relative to THIS FILE.";
        x.kind = Kind::Str;
        p.push_back(std::move(x));
    }
    {
        Property x;
        x.name    = "readonly";
        x.help    = "Write-protect the disk. A write gives WRITE FAULT";
        x.kind    = Kind::Bool;
        x.aliases = {"writeprotect"};
        p.push_back(std::move(x));
    }
    p.push_back(typeKey());
    p.push_back(sectorsKey());
    {
        Property x;
        x.name = "create";
        x.help = "Make the disk file if it is not there: a new disk, filled with E5";
        x.kind = Kind::Bool;
        p.push_back(std::move(x));
    }
    return p;
}

bool CompuProDisk2Board::addSubUnit(const std::string& table, const KeyValues& kv,
                                    std::string& err) {
    if (table != "drive") {
        err = type() + " has no [[board." + table + "]] table";
        return false;
    }
    int         unit = -1;
    std::string path, model, sectors;
    bool        ro = false, create = false;
    auto flag = [](const std::string& v) {
        Value       bv;
        std::string e;
        return parseValue(v, Kind::Bool, bv, e) && bv.b();
    };
    for (const auto& [k, v] : kv) {
        if (k == "unit") unit = std::stoi(v);
        else if (k == "mount") path = v;
        else if (k == "readonly") ro = flag(v);
        else if (k == "create") create = flag(v);
        else if (k == "type") model = v;
        else if (k == "sectors") sectors = v;
    }
    if (unit < 0) {
        err = "[[board.drive]] needs a `unit`";
        return false;
    }
    Drive& d = drive_[(size_t)unit];
    if (path.empty()) {
        if (!model.empty()) d.model = lowerAscii(model);
        if (!sectors.empty()) d.sectors = std::stoi(sectors);
        return true;
    }
    // `create`: an empty file at the place mount() opens, never over a file that is there.
    if (create) {
        const std::string rp = resolvePath(path);
        std::error_code   ec;
        if (!std::filesystem::exists(rp, ec)) {
            std::string cerr;
            if (!writeHostFile(rp, {}, cerr)) {
                err = cerr + pathNote(path);
                return false;
            }
        }
    }
    return mountWith("drive" + std::to_string(unit), path, ro,
                     {{"type", model}, {"sectors", sectors}}, err);
}

std::vector<Board::SubUnit> CompuProDisk2Board::subUnits() const {
    std::vector<SubUnit> out;
    for (int i = 0; i < kDrives; ++i) {
        const Drive& d = drive_[(size_t)i];
        const bool   custom = d.model != "m20" || d.sectors != 11;
        if (!d.img && !custom) continue;
        SubUnit su;
        su.table = "drive";
        su.fields.push_back({"unit", std::to_string(i), false});
        if (d.model != "m20") su.fields.push_back({"type", d.model, true});
        if (d.sectors != 11) su.fields.push_back({"sectors", std::to_string(d.sectors), false});
        if (d.img) {
            su.fields.push_back({"mount", d.path, true});
            if (d.img->readOnly()) su.fields.push_back({"readonly", "true", false});
        }
        out.push_back(std::move(su));
    }
    return out;
}

std::vector<std::string> CompuProDisk2Board::drainLog() {
    std::vector<std::string> out = std::move(log_);
    log_.clear();
    for (auto& s : out) s = id + ": " + s;
    return out;
}

// ---------------------------------------------------------------------------
// SNAPSHOT / RESTORE.
// ---------------------------------------------------------------------------
void CompuProDisk2Board::serialize(StateWriter& w) const {
    Board::serialize(w);
    for (uint8_t v : {ctl_, driveReg_, cylReg_, headReg_, secReg_, latched_, op_}) w.u8(v);
    for (bool v : {attn_, busy_, hung_, timeout_, crc_, overrun_, xfer_.active, xfer_.toMemory})
        w.boolean(v);
    w.u8((uint8_t)unit_);
    w.u32((uint32_t)xfer_.done);
    w.blob(xfer_.buf);
    for (const Drive& d : drive_) {
        w.u32((uint32_t)d.pos);
        w.boolean(d.fault);
        w.u64(d.seekEnd);
    }
}

void CompuProDisk2Board::deserialize(StateReader& r) {
    Board::deserialize(r);
    for (uint8_t* v : {&ctl_, &driveReg_, &cylReg_, &headReg_, &secReg_, &latched_, &op_})
        *v = r.u8();
    for (bool* v : {&attn_, &busy_, &hung_, &timeout_, &crc_, &overrun_, &xfer_.active,
                    &xfer_.toMemory})
        *v = r.boolean();
    unit_      = r.u8() & 0x03;
    xfer_.done = r.u32();
    xfer_.buf  = r.blob();
    if (xfer_.done >= xfer_.buf.size()) xfer_.active = false;
    for (Drive& d : drive_) {
        d.pos     = (int)r.u32();
        d.fault   = r.boolean();
        d.seekEnd = r.u64();
    }
    intChanged();
    holdChanged();
}

} // namespace altair
