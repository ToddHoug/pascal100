#pragma once
//
// CompuPro DISK 2 (board 177) -- the hard disk controller for Fujitsu 2300-series drives.
// See reference/CompuPro Disk 2 & Selector Channel.md; page numbers below are the manual's.
//
// TWO PORTS (p. 5), base set by SW2 (default C8):
//   BASE+0  OUT = CTL, IN = STATUS
//   BASE+1  OUT = the data register OP1/OP0 picks (DRIVE, CYLINDER, HEAD, SECTOR);
//           IN = one STEP pulse to the selected drives, in the OP2 direction. The read
//           returns no data: the board drives nothing, so snoop() takes it.
//
// IT NEEDS A SELECTOR CHANNEL (boards/compupro-selch.h). For a command that moves data,
// the board pulls pHOLD at its TMA priority (SW1-2..5, `dma_priority`, default 10, CompuPro's
// SELPRI). Granted, it finds a Selector Channel on the bus that serves that priority, and has
// it run one cycle per byte. With none, the bytes go nowhere and the command ends with
// OVER RUN (p. 9: "the SELECTOR CHANNEL programming ... should be verified").
//
// A SECTOR IS FOUND AT ONCE (the uPD765 rule: no rotation, no byte timing). A command runs
// when the CTL write starts it; its data moves at the next instruction boundary as one
// burst (the real board holds the bus for the whole data field), one 3-state bus cycle a
// byte; then ATTN.
//
// A SEEK TAKES TIME. Each step pulse drops SEEK COMPLETE on the drive, and it comes back
// 30 ms of emulated time after the last pulse (the M2302B buffers pulses and seeks after the
// last; 30 ms is its minimum positioning time). CompuPro's DISK2.CMD needs this: after one
// step it waits for SEEK COMPLETE* to go high, else "PANIC: 20 pin cable plugged in
// backwards". The heads are on the new cylinder at once; a command does not wait for them.
//
// THE IMAGE HOLDS SECTORS, NOT HEADERS. Every track is taken to carry the headers CompuPro
// formats: (cylinder, head, 0 .. sectors-1). READ/WRITE DATA find a sector only when the
// CYLINDER register equals the drive's real cylinder (low 8 bits, as the header byte), HEAD
// equals the head lines (DRIVE bits 3-0) and SECTOR is on the track; otherwise TIME OUT, as
// on the board when a seek was missed. READ HEADER gives (cylinder, head, 0). WRITE HEADER
// takes any order of the standard headers (skew); another header is logged and not kept.
//
// FAILURES THE REAL BOARD HAS:
//   - A command to a drive (US1/US0) with no disk hangs: the state machine is clocked by
//     the drive (p. 16). It stays busy until a bus RESET. Logged once.
//   - US1/US0 not the drive the DRIVE register selects: the data comes from a drive that is
//     not reading. TIME OUT, logged.
//   - A write to a read-only disk: WRITE FAULT, which only FAULT CLR clears (p. 10). While
//     it is set the drive takes no command: TIME OUT.
//
// ATTN* (p. 3): asserted (low) when a command ends and after RESET; the STATUS register is
// latched while it is asserted. A CTL write with D7 = 1 releases it. The jumper J7-J14 puts
// it on VI0-VI7, J15 on pINT (`interrupt`). CompuPro's MP/M 8-16 XIOS20 waits for it on VI1.

#include "core/board.h"
#include "host/disk.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace altair {

class SelectorChannelBoard;

class CompuProDisk2Board : public Board {
public:
    CompuProDisk2Board();

    std::string type() const override { return "disk2"; }

    // ---- the bus ----
    bool    decodes(const BusCycle& c) const override;
    uint8_t read(const BusCycle& c) override;
    void    write(const BusCycle& c) override;
    void    snoop(const BusCycle& c) override;
    bool    wantsSnoop() const override { return true; }

    bool    assertsInt() const override;
    uint8_t assertsVi() const override;

    // ---- pHOLD: a command that moves data (HAS-A Mover, as the Disk 1) ----
    bool       requestsBus() const override { return xfer_.active; }
    BusMaster* busMaster() override { return &mover_; }
    int        tmaPriority() const override { return priority_; }

    void reset(Reset r) override;
    void power() override;

    std::vector<Property> properties() override;
    std::vector<MapEntry> ioMap() const override;

    std::vector<UnitDef> units() const override;
    bool mount(const std::string& unit, const std::string& path, bool ro, std::string& err) override;
    bool unmount(const std::string& unit, std::string& err) override;
    std::vector<Property> mountKeys(const std::string& unit) const override;
    bool mountWith(const std::string& unit, const std::string& path, bool ro, const KeyValues& keys,
                   std::string& err) override;

    std::vector<std::string> subUnitTables() const override { return {"drive"}; }
    std::vector<Property>    subUnitProperties(const std::string& table) const override;
    std::vector<SubUnit>     subUnits() const override;

    std::vector<std::string> drainLog() override;

    void serialize(StateWriter& w) const override;
    void deserialize(StateReader& r) override;

    // For tests.
    bool busy() const { return busy_; }
    bool hung() const { return hung_; }
    int  cylinder(int drive) const { return drive_[(size_t)drive].pos; }

    static constexpr int kDrives = 4;

protected:
    bool addSubUnit(const std::string& table, const KeyValues& kv, std::string& err) override;

private:
    struct Drive {
        std::unique_ptr<DiskImage> img;
        std::string path;           // as WRITTEN (round-trips through SAVE)
        std::string model = "m20";  // `type`: m20 (M2302B) or m10 (M2301B)
        int         sectors = 11;   // the drive's sector switches (Fujitsu Table 3.7)
        int         pos     = 0;    // the cylinder under the heads
        bool        fault   = false;
        uint64_t    seekEnd = 0;    // T-state SEEK COMPLETE comes back (30 ms after a step)
    };

    // One command's data, moved a byte per granted step.
    struct Xfer {
        bool                 active = false;
        bool                 toMemory = false;
        size_t               done = 0;
        std::vector<uint8_t> buf;
    };

    void    writeCtl(uint8_t v);
    void    writeData(uint8_t v);
    void    step();
    void    start();
    void    finishTransfer();
    void    complete();
    void    timeOut();
    uint8_t liveStatus() const;
    int     selectedDrive() const;  // the one drive DRIVE bits 7-4 select; -1 none, -2 more
    bool    headerMatches(const Drive& d) const;
    SelectorChannelBoard* channel(Bus& bus) const;
    Property typeKey() const;
    Property sectorsKey() const;

    StepResult transferOne(Bus& bus);

    // HAS-A, NOT IS-A: Machine::master()'s dynamic_cast must not take the board for a CPU.
    struct Mover : public BusMaster {
        CompuProDisk2Board* d = nullptr;
        StepResult step(Bus& bus) override { return d->transferOne(bus); }
    } mover_;

    std::array<Drive, kDrives> drive_;

    // ---- straps ----
    uint8_t   port_       = 0xC8;  // SW2-1..7 = A7..A1
    int       priority_   = 10;    // SW1-2..5; CompuPro's SELPRI
    int       sectorSize_ = 1024;  // SW1-6..10
    IrqJumper irq_        = IrqJumper::None;

    // ---- runtime (snapshot) ----
    uint8_t ctl_ = 0, driveReg_ = 0, cylReg_ = 0, headReg_ = 0, secReg_ = 0;
    uint8_t latched_ = 0;       // STATUS as it was when ATTN came up
    bool    attn_    = true;    // ATTN asserted (ATTN* low)
    bool    busy_    = false;
    bool    hung_    = false;
    bool    timeout_ = false, crc_ = false, overrun_ = false;
    uint8_t op_      = 0;       // the command running
    int     unit_    = 0;       // its US1/US0
    Xfer    xfer_;

    std::vector<std::string> log_;
};

} // namespace altair
