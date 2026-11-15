// CompuPro DISK 2 + Selector Channel (src/boards/compupro-disk2.h, compupro-selch.h), through
// real bus cycles. The command sequence is the one CompuPro's TMXDISK2.DVR (CP/M 8-16 V1.1R)
// sends: select the drive and head (CTL 80+unit, DATA), step with IN BASE+1, load CYLINDER /
// HEAD / SECTOR, IN F0 then four OUTs to the Selector Channel (A5 = read, 25 = write, for
// priority 10), then the command (C8 = READ DATA, D0 = WRITE DATA), and a poll of STATUS
// bit 7 -- run by a Z80, so the DMA bytes move in the run loop as the bus is granted.

#include "boards/compupro-disk1.h"
#include "boards/compupro-disk2.h"
#include "boards/compupro-selch.h"
#include "boards/s100-memory.h"
#include "core/bus.h"
#include "core/debug.h"
#include "core/machine.h"
#include "core/statefile.h"
#include "host/media.h"
#include "test.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace altair;

namespace {

constexpr uint8_t CTL = 0xC8, STAT = 0xC8, DATA = 0xC9, SC = 0xF0;

// CTL bytes, as COMPUPRO.EQU spells them.
constexpr uint8_t STRB = 0x80, CYLR = 0x88, HEADR = 0x90, SECR = 0x98, SIN = 0xA0, SOU = 0x80,
                  RST = 0x04, READ = 0xC8, WRITE = 0xD0, RDHDR = 0xE0, WRHDR = 0xD8, NUL = 0xC0;

constexpr uint64_t kM20 = 244ull * 8 * 11 * 1024;  // 21,987,328
constexpr uint64_t kM10 = 244ull * 4 * 11 * 1024;

// Every byte of a disk is its own file offset, stamped, so a sector from the wrong place
// never matches by accident.
uint8_t stamp(uint64_t i) { return (uint8_t)((i + i / 1024 * 7) & 0xFF); }

uint64_t offsetOf(int cyl, int head, int sec, int heads = 8) {
    return (((uint64_t)cyl * (uint64_t)heads + (uint64_t)head) * 11 + (uint64_t)sec) * 1024;
}

// The media every mount gets: a ramp of `bytes`, or an empty file (a new disk).
// g_media is the last one made, to count its syncs.
MemoryMedia* g_media = nullptr;
void withDisk(uint64_t bytes) {
    setMediaResolver([bytes](const std::string& path, bool ro, std::string&) {
        std::vector<uint8_t> d((size_t)bytes);
        for (size_t i = 0; i < d.size(); ++i) d[i] = stamp(i);
        auto m  = std::make_unique<MemoryMedia>(path, std::move(d), ro);
        g_media = m.get();
        return m;
    });
}

struct Rig {
    Machine               m;
    CompuProDisk2Board*   hd = nullptr;
    SelectorChannelBoard* sc = nullptr;

    explicit Rig(bool withSelch = true) {
        std::string err;
        auto* mem = dynamic_cast<MemoryBoard*>(m.add("memory", "mem0", err));
        Region r;
        r.kind = RegionKind::Ram;
        r.at   = 0x0000;
        r.size = 0x10000;
        mem->addRegion(r, err);
        setProperty(*mem, "fill", "zero", err);
        mem->power();
        hd = dynamic_cast<CompuProDisk2Board*>(m.add("disk2", "hd0", err));
        if (withSelch) sc = dynamic_cast<SelectorChannelBoard*>(m.add("selch", "sc0", err));
        m.add("z80", "cpu0", err);
        m.cpu()->reset(Reset::PowerOn);
        hd->power();
        if (sc) sc->power();
    }

    uint8_t in(uint8_t p) { return m.bus.ioRead(p); }
    void    out(uint8_t p, uint8_t v) { m.bus.ioWrite(p, v); }
    uint8_t mem(uint16_t a) { return m.bus.memRead(a); }

    bool decodes(Cycle type, uint8_t port) const {
        BusCycle c;
        c.type = type;
        c.addr = port;
        return hd->decodes(c);
    }

    // 30 ms of emulated time: a seek's length after the last step pulse.
    uint64_t seekTime() { return m.clock.tStatesPer(1000) * 30; }
    void     settle() { m.clock.advance(seekTime()); }

    // Drive `unit`, head `head`, then `n` steps (n > 0 inward).
    void select(int unit, int head) {
        out(CTL, (uint8_t)(STRB | unit));
        out(DATA, (uint8_t)((0x10 << unit) | head));
    }
    // By default it then waits out the seek, as every CompuPro driver does.
    void stepBy(int unit, int n, bool wait = true) {
        out(CTL, (uint8_t)((n > 0 ? SIN : SOU) | unit));
        for (int i = 0; i < (n > 0 ? n : -n); ++i) in(DATA);
        if (wait) settle();
    }
    void target(int cyl, int head, int sec) {
        out(CTL, CYLR); out(DATA, (uint8_t)cyl);
        out(CTL, HEADR); out(DATA, (uint8_t)head);
        out(CTL, SECR); out(DATA, (uint8_t)sec);
    }
    void program(uint32_t addr, uint8_t mode) {
        in(SC);
        out(SC, (uint8_t)(addr >> 16));
        out(SC, (uint8_t)(addr >> 8));
        out(SC, (uint8_t)addr);
        out(SC, mode);
    }

    // The command from a Z80 that then polls STATUS bit 7 and halts:
    //   8000 MVI A,cmd / OUT C8 / 8004 IN C8 / ORA A / JM 8004 / HLT
    uint8_t command(uint8_t cmd) {
        uint16_t pc = 0x8000;
        for (uint8_t b : {(uint8_t)0x3E, cmd, (uint8_t)0xD3, CTL, (uint8_t)0xDB, STAT, (uint8_t)0xB7,
                          (uint8_t)0xFA, (uint8_t)0x04, (uint8_t)0x80, (uint8_t)0x76})
            m.bus.memWrite(pc++, b);
        m.cpu()->reset(Reset::Bus);
        m.cpu()->setPc(0x8000);
        m.debug.run(0);
        return in(STAT);
    }

    bool hasLog(const std::string& part) {
        for (const auto& s : hd->drainLog())
            if (s.find(part) != std::string::npos) return true;
        return false;
    }
};

} // namespace

void test_disk2() {
    SECTION("disk2: the port map");
    {
        Rig k;
        CHECK(k.decodes(Cycle::IoRead, 0xC8) && k.decodes(Cycle::IoWrite, 0xC8), "C8 is CTL / STATUS");
        CHECK(k.decodes(Cycle::IoWrite, 0xC9), "an OUT to C9 is a data register");
        CHECK(!k.decodes(Cycle::IoRead, 0xC9), "an IN from C9 drives nothing: it is a STEP pulse");
        CHECK(!k.decodes(Cycle::IoRead, 0xCA) && !k.decodes(Cycle::IoWrite, 0xC7), "two ports and no more");
        std::string err;
        CHECK(!setProperty(*k.hd, "port", "C9", err), "an odd base is refused: SW2 sets A7-A1");
        CHECK(setProperty(*k.hd, "port", "40", err) && k.decodes(Cycle::IoRead, 0x40), "the base moves to 40");
    }

    SECTION("disk2: ATTN after reset, the interrupt jumper");
    {
        Rig k;
        CHECK(k.in(STAT) == 0x0F, "after power-on ATTN is asserted (D7 = 0); no drive selected: 0F");
        CHECK(k.hd->assertsVi() == 0 && !k.hd->assertsInt(), "no jumper (the default): no interrupt");
        std::string err;
        setProperty(*k.hd, "interrupt", "vi1", err);
        CHECK(k.hd->assertsVi() == 0x02, "J8: ATTN on VI1, as MP/M's XIOS20 wants");
        k.out(CTL, STRB);
        CHECK(k.in(STAT) == 0x8F && k.hd->assertsVi() == 0, "CTL D7 = 1 releases ATTN: transparent, no interrupt");
        setProperty(*k.hd, "interrupt", "int", err);
        k.hd->reset(Reset::Bus);
        CHECK(k.hd->assertsInt() && (k.in(STAT) & 0x80) == 0, "J15: pINT, and RESET asserts ATTN again");
    }

    SECTION("disk2: mounting");
    {
        std::string err;
        withDisk(1000);
        Rig k;
        CHECK(!k.hd->mount("drive0", "x.dsk", false, err), "a size that is no M20 is refused");
        CHECK(err.find("21987328") != std::string::npos, "...and the error gives the M20 size");
        withDisk(kM20);
        CHECK(k.hd->mount("drive0", "hd.dsk", false, err), "an M20 image mounts");
        withDisk(kM10);
        CHECK(!k.hd->mount("drive1", "hd.dsk", false, err), "an M10-size image is not an M20");
        CHECK(k.hd->mountWith("drive1", "hd.dsk", false, {{"type", "m10"}}, err), "...with type = m10 it mounts");
        CHECK(!setProperty(*k.hd, "sector_size", "512", err), "the sector size cannot change under a mounted disk");
        withDisk(0);
        CHECK(k.hd->mount("drive2", "new.dsk", false, err), "an empty file is a new disk");
        CHECK(k.hasLog("new disk, 21987328 bytes of E5"), "...filled to the M20 size with E5");
        CHECK(!k.hd->mount("drive3", "new.dsk", true, err), "...but not a read-only one");
    }

    SECTION("disk2: drive status and stepping");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 0);
        CHECK(k.in(STAT) == 0x82, "ready, seek complete, no fault, track 0 (with ATTN released: 82)");
        k.stepBy(0, 5, false);
        CHECK(k.hd->cylinder(0) == 5 && k.in(STAT) == 0x87,
              "five steps in: cylinder 5, TRACK 0* off, SEEK COMPLETE* high while the heads move");
        k.m.clock.advance(k.seekTime() - 1);
        CHECK(k.in(STAT) == 0x87, "...still seeking 1 T-state short of 30 ms");
        k.m.clock.advance(1);
        CHECK(k.in(STAT) == 0x83, "...and on track 30 ms after the last pulse");
        k.stepBy(0, 1, false);
        k.m.clock.advance(k.seekTime() / 2);
        k.in(DATA);
        k.m.clock.advance(k.seekTime() / 2 + 1);
        CHECK(k.in(STAT) == 0x87, "a second pulse restarts the 30 ms (the drive seeks after the last)");
        k.settle();
        CHECK(k.hd->cylinder(0) == 7 && k.in(STAT) == 0x83, "...then SEEK COMPLETE");
        k.stepBy(0, -2);
        k.stepBy(0, -9);
        CHECK(k.hd->cylinder(0) == 0, "steps out stop at track 0");
        k.stepBy(0, 300);
        CHECK(k.hd->cylinder(0) == 243, "steps in stop at the last cylinder, 243");
        k.out(CTL, STRB);
        k.out(DATA, 0x00);
        k.stepBy(0, -1);
        CHECK(k.hd->cylinder(0) == 243, "no drive selected: a step reaches nothing");
        CHECK(k.in(STAT) == 0x8F, "...and the drive lines float high");
    }

    SECTION("disk2: READ DATA through the Selector Channel");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 2);
        k.stepBy(0, 3);
        k.target(3, 2, 4);
        k.program(0x002000, 0xA5);
        CHECK(k.sc->serves(10), "MODE A5: write to memory, count up, priority 10 (complement 5)");
        const uint8_t st = k.command(READ);
        CHECK(st == 0x03, "a good read off track 0 reads 03 (TMXDISK2: (st & 7E) ^ 02 == 0)");
        bool same = true;
        const uint64_t off = offsetOf(3, 2, 4);
        for (int i = 0; i < 1024; ++i) same = same && k.mem((uint16_t)(0x2000 + i)) == stamp(off + (uint64_t)i);
        CHECK(same, "the 1024 bytes of cylinder 3 head 2 sector 4 are at 2000");
        CHECK(k.sc->address() == 0x002400, "the Selector Channel counted to 2400");
        k.stepBy(0, -3);
        k.target(0, 2, 0);
        k.program(0x002000, 0xA5);
        CHECK(k.command(READ) == 0x02, "on track 0 a good read reads 02");
    }

    SECTION("disk2: WRITE DATA, then read it back");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        for (int i = 0; i < 1024; ++i) k.m.bus.memWrite((uint16_t)(0x3000 + i), (uint8_t)(i * 7));
        k.select(0, 7);
        k.stepBy(0, 100);
        k.target(100, 7, 10);
        k.program(0x003000, 0x25);
        const int syncs = g_media->syncs();
        CHECK(k.command(WRITE) == 0x03, "a good write reads 03");
        CHECK(g_media->syncs() == syncs + 1, "...and the sector goes to the host file at once (one sync)");
        k.target(100, 7, 10);
        k.program(0x004000, 0xA5);
        CHECK(k.command(READ) == 0x03, "...and reading it back is good too");
        bool same = true;
        for (int i = 0; i < 1024; ++i) same = same && k.mem((uint16_t)(0x4000 + i)) == (uint8_t)(i * 7);
        CHECK(same, "the sector holds what was written");
    }

    SECTION("disk2: TIME OUT -- no such header");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 0);
        k.target(5, 0, 0);  // the heads are at cylinder 0: the seek was missed
        k.program(0x002000, 0xA5);
        CHECK(k.command(READ) == 0x42, "a CYLINDER the heads are not on: TIME OUT (42)");
        CHECK(k.mem(0x2000) == 0x00, "...and nothing moved");
        CHECK(k.sc->address() == 0x002000, "...the Selector Channel did not count");
        k.target(0, 1, 0);
        CHECK(k.command(READ) == 0x42, "a HEAD register that is not the head lines: TIME OUT");
        k.target(0, 0, 11);
        CHECK(k.command(READ) == 0x42, "sector 11 on an 11-sector track (0-10): TIME OUT");
        k.target(0, 0, 10);
        CHECK(k.command(READ) == 0x02, "sector 10 is the last one");
    }

    SECTION("disk2: OVER RUN -- no Selector Channel for this priority");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 0);
        k.target(0, 0, 0);
        k.program(0x002000, 0xA5);
        k.in(SC);  // disabled again
        CHECK(k.command(READ) == 0x12, "the Selector Channel disabled: OVER RUN (12)");
        k.program(0x002000, 0xA4);  // the complement of 11, not 10
        CHECK(!k.sc->serves(10), "MODE A4 is for priority 11");
        CHECK(k.command(READ) == 0x12, "...so the DISK 2 at priority 10 gets no cycles: OVER RUN");
        CHECK(k.mem(0x2000) == 0x00, "...and nothing reached memory");
        setProperty(*k.hd, "dma_priority", "11", err);
        k.program(0x002000, 0xA4);
        CHECK(k.command(READ) == 0x02, "dma_priority = 11 matches it");
    }
    {
        withDisk(kM20);
        Rig k(false);
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 0);
        k.target(0, 0, 0);
        CHECK(k.command(READ) == 0x12, "no Selector Channel in the machine at all: OVER RUN");
    }

    SECTION("disk2: the interrupt when a command ends");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        setProperty(*k.hd, "interrupt", "vi1", err);
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 0);
        CHECK(k.hd->assertsVi() == 0, "ATTN released by the CTL writes");
        k.target(0, 0, 0);
        k.program(0x002000, 0xA5);
        k.command(READ);
        CHECK(k.hd->assertsVi() == 0x02, "the read ends: ATTN on VI1");
        k.out(CTL, STRB);
        CHECK(k.hd->assertsVi() == 0, "D2_STRB releases it, as TMXDISK2 does after each command");
        k.out(CTL, NUL);
        CHECK(k.hd->assertsVi() == 0x02 && !k.hd->busy(), "NULL ends at once (no rotation): ATTN again");
    }

    SECTION("disk2: the hang, and US1/US0 against the DRIVE register");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(1, 0);
        k.out(CTL, READ | 1);  // drive 1 has no disk
        CHECK(k.hd->busy() && k.hd->hung(), "a command to an empty drive never ends");
        CHECK((k.in(STAT) & 0x80) != 0, "STATUS stays busy (D7 = 1)");
        CHECK(k.hasLog("waits for ever"), "...and the board says why");
        k.hd->reset(Reset::Bus);
        CHECK(!k.hd->busy() && !k.hd->hung(), "a bus RESET stops it");
        k.select(0, 0);
        k.target(0, 0, 0);
        k.program(0x002000, 0xA5);
        k.out(CTL, STRB | 1);
        k.out(CTL, READ | 1);  // US = 1, but DRIVE selects drive 0 -- and drive 1 is empty
        CHECK(k.hd->hung(), "US1/US0 names the clock source: drive 1, empty, hangs");
        k.hd->reset(Reset::Bus);
        withDisk(kM20);
        k.hd->mount("drive1", "hd1.dsk", false, err);
        k.select(0, 0);
        CHECK(k.command(READ | 1) == 0x42, "US = 1 with DRIVE selecting drive 0: TIME OUT");
        CHECK(k.hasLog("the DRIVE register selects another"), "...and the board says why");
    }

    SECTION("disk2: a read-only disk gives WRITE FAULT");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", true, err);
        k.select(0, 0);
        k.target(0, 0, 0);
        k.program(0x002000, 0x25);
        CHECK(k.command(WRITE) == 0x00, "WRITE FAULT* low on track 0: 00");
        CHECK(k.hasLog("read-only: WRITE FAULT"), "...and the board says why");
        k.program(0x002000, 0xA5);
        CHECK(k.command(READ) == 0x40, "a drive with WRITE FAULT takes no command: TIME OUT");
        k.out(CTL, STRB | RST);
        CHECK(k.in(STAT) == 0xC2, "FAULT CLR (D2_RST) clears it: WRITE FAULT* is 1 again "
                                  "(TIME OUT stays until the next command starts)");
        k.program(0x002000, 0xA5);
        CHECK(k.command(READ) == 0x02, "...and the drive reads again");
    }

    SECTION("disk2: WRITE HEADER and READ HEADER");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 3);
        k.stepBy(0, 9);
        const uint8_t hdrs[] = {9, 3, 7, 9, 3, 99};
        for (int i = 0; i < 6; ++i) k.m.bus.memWrite((uint16_t)(0x5000 + i), hdrs[i]);
        k.program(0x005000, 0x25);
        CHECK(k.command(WRHDR) == 0x03, "WRITE HEADER (9, 3, 7): done");
        CHECK(k.sc->address() == 0x005003, "...the Selector Channel moved three bytes (p. 10)");
        CHECK(k.hd->drainLog().empty(), "...a standard header in any order is taken quietly");
        CHECK(k.command(WRHDR) == 0x03, "the next WRITE HEADER reads on from 5003 without reprogramming");
        CHECK(k.hasLog("is not the standard"), "(9, 3, 99) is no sector of this track: logged, not kept");
        k.program(0x006000, 0xA5);
        CHECK(k.command(RDHDR) == 0x03, "READ HEADER: done");
        CHECK(k.mem(0x6000) == 9 && k.mem(0x6001) == 3 && k.mem(0x6002) == 0, "...gives (cylinder, head, 0)");
    }

    SECTION("selch: programming");
    {
        Rig k;
        CHECK(!k.sc->armed(), "it powers up disabled");
        k.out(SC, 0x12); k.out(SC, 0x34); k.out(SC, 0x56);
        CHECK(!k.sc->armed(), "three OUTs are not enough");
        k.out(SC, 0x25);
        CHECK(k.sc->armed() && k.sc->address() == 0x123456 && k.sc->mode() == 0x25,
              "the fourth enables it: A23-A16, A15-A8, A7-A0, MODE");
        k.in(SC);
        CHECK(!k.sc->armed(), "an IN disables it");
        k.program(0xABCDEF, 0x05);  // read from memory, count DOWN
        k.m.bus.memWriteX(0xABCDEF, 0x77);
        CHECK(k.sc->cycle(k.m.bus, SelectorChannelBoard::kFloating) == 0x77 && k.sc->address() == 0xABCDEE,
              "U/D* = 0 counts down after the byte");
        k.program(0x0000C0, 0x45);  // I/O read
        k.sc->cycle(k.m.bus, SelectorChannelBoard::kFloating);
        CHECK(k.sc->address() == 0x0000C0, "an I/O cycle does not count");
        k.sc->reset(Reset::Bus);
        CHECK(!k.sc->armed(), "RESET disables it");
    }

    SECTION("disk2: two bus masters, two priorities");
    {
        Rig k;
        std::string err;
        k.m.add("disk1", "fd0", err);
        std::string why;
        CHECK(!k.m.tmaConflict(why), "a Disk 1 at 15 and a DISK 2 at 10 share the bus");
        setProperty(*k.hd, "dma_priority", "15", err);
        CHECK(k.m.tmaConflict(why), "...both at 15 is refused");
    }

    SECTION("disk2: SNAPSHOT / RESTORE");
    {
        withDisk(kM20);
        Rig k;
        std::string err;
        k.hd->mount("drive0", "hd.dsk", false, err);
        k.select(0, 4);
        k.stepBy(0, 42);
        k.target(42, 4, 6);
        StateWriter w;
        k.hd->serialize(w);
        k.sc->serialize(w);
        Rig k2;
        k2.hd->mount("drive0", "hd.dsk", false, err);
        StateReader r(w.data());
        k2.hd->deserialize(r);
        k2.sc->deserialize(r);
        k2.m.clock.advance(k.m.clock.now());  // RESTORE brings the clock back too
        CHECK(r.ok() && k2.hd->cylinder(0) == 42, "the heads come back at cylinder 42");
        k2.program(0x002000, 0xA5);
        CHECK(k2.command(READ) == 0x03, "...with DRIVE, CYLINDER, HEAD and SECTOR: the read finds 42/4/6");
        CHECK(k2.mem(0x2000) == stamp(offsetOf(42, 4, 6)), "...and it is the right sector");
    }
}
