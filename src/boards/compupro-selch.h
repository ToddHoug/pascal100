#pragma once
//
// CompuPro Selector Channel (board 172) -- the DMA address and cycle half of a temporary
// master. See reference/CompuPro Disk 2 & Selector Channel.md, "Selector Channel section".
//
// A SELECTOR CHANNEL NEVER ASKS FOR THE BUS. A device controller does (today the DISK 2,
// boards/compupro-disk2.h): it pulls pHOLD with its own TMA priority, and the Selector
// Channel serves the controller whose priority matches the low nibble of its MODE byte
// (stored complemented, p. 32). It holds the 24-bit address, drives the status and the
// strobes, and counts the address after each byte. The controller only says "a byte now"
// (pSYNC) and drives or takes the data. So in the model the controller is the BusMaster,
// and it finds a Selector Channel on the bus that serves its priority (serves()) and has
// it run each cycle (cycle()).
//
// PROGRAMMING (p. 32). An IN from the port disables the board and restarts the count; it
// returns no data (the board drives nothing, so the bus floats). Then exactly four OUTs:
// A23-A16, A15-A8, A7-A0, MODE. The MODE register and the three address counters are one
// four-stage shift register (p. 33): each OUT shifts MODE -> low -> high -> extended, so
// after four the first byte is in the extended counter and the last is MODE. The board is
// enabled after the fourth write. It powers up, and comes out of RESET*, disabled.
//
// MODE: D7 W/R* (write to / read from the slave), D6 IO/M* (I/O cycles do not count),
// D5 U/D* (memory address up / down), D4 WAIT (one wait state per cycle), D3-D0 the
// complement of the controller's DMA priority.
//
// NOT MODELED, AND WHY EACH IS SAFE TO LEAVE OUT:
//   - THE BOOT EPROM (SW1-1, a 2716 over PHANTOM*, J1-J4 waits, J8-J10 segment jumpers).
//     CompuPro's MP/M 8-16 runs the board with SW1-1 OFF and boots its hard disk from a
//     floppy. Nothing we run uses it.
//   - WAIT STATES (SW1-2, MODE D4). No slow memory: nothing to wait for. The bit is kept
//     and ignored.

#include "core/board.h"

#include <cstdint>
#include <string>
#include <vector>

namespace altair {

class SelectorChannelBoard : public Board {
public:
    std::string type() const override { return "selch"; }

    // ---- the bus: one port. OUT shifts a byte in; IN (seen by snoop()) disables. ----
    bool    decodes(const BusCycle& c) const override;
    uint8_t read(const BusCycle&) override { return 0; }  // never decoded: see decodes()
    void    write(const BusCycle& c) override;
    void    snoop(const BusCycle& c) override;
    bool    wantsSnoop() const override { return true; }

    void reset(Reset r) override;
    void power() override { reset(Reset::PowerOn); }

    std::vector<Property> properties() override;
    std::vector<MapEntry> ioMap() const override;

    void serialize(StateWriter& w) const override;
    void deserialize(StateReader& r) override;

    // ---- the controller's side ----
    // Is the board enabled, and programmed for the controller at this TMA priority?
    bool serves(int priority) const;

    // One bus cycle for the controller, at the address, as the MODE byte says. `out` is the
    // byte on the data-out bus (the controller drives it when it writes to memory); the
    // result is the byte on the data-in bus (what a memory or I/O read returned). After a
    // memory cycle the address counts up or down; an I/O cycle leaves it alone.
    uint8_t cycle(Bus& bus, uint8_t out);

    // Nothing drives a data bus: the controller passes this as `out` when it is not
    // driving, and cycle() returns it for the data-in bus on a write. It is the floating
    // bus of DESIGN.md 4.6.1, not a value either board makes up.
    static constexpr uint8_t kFloating = 0xFF;

    // For tests and SHOW.
    bool     armed() const { return armed_; }
    uint32_t address() const { return addr_; }
    uint8_t  mode() const { return mode_; }

private:
    // ---- strap ----
    uint8_t port_ = 0xF0;  // SW1-3..10 = A7..A0; F0 is CompuPro's (COMPUPRO.EQU SELCHAN)

    // ---- runtime (snapshot) ----
    uint32_t addr_   = 0;      // A23-A0
    uint8_t  mode_   = 0;
    int      writes_ = 0;      // OUTs since the last IN or reset, up to 4 (U10)
    bool     armed_  = false;  // enabled: four OUTs since the last IN or reset
};

} // namespace altair
