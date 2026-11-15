#pragma once
//
// The Digicomp Research Pascal-100 -- docs/boards/digicomp-pascal100.md.
//
// TWO PROCESSORS ON ONE CARD, ONE ACTIVE (DESIGN.md 3.0.1): a Z80 (unit `z80`) and the
// Western Digital MicroEngine (unit `me`, the P-machine core of cpu/cpuPmachine.h). The
// processor-select flip-flop U208B picks which one runs; the other is in HOLD and resumes
// where it left off. The card is a Board as well as a BusMaster: it answers its own two
// I/O ports (the System Control Port and the Map Output Port).
//
// Sources: the Pascal-100 User's Manual (Reference/pascal100/, ch. II B-F and ch. IV I-P;
// distilled in reference/Digicomp Research Pascal-100.md). Nothing from another emulator.
//
// WHAT THE CARD DOES BETWEEN A CORE AND THE BUS:
//  - the Standard Map (16 x 4 bits, 8K pages, 128K physical), per processor on or off;
//  - the I/O address mirror: the port goes out on A0-A7 AND A8-A15 for both processors;
//  - the MicroEngine's memory-mapped I/O: logical word FFpp / FEpp = I/O port pp;
//  - the MicroEngine's 16-bit memory words: one SIXTN cycle, or two byte cycles, even first;
//  - jump-on-reset: the Z80's first three reads after a reset get C3 00 X0.
// The Z80 side goes through the bus's AddressTranslator hooks (core/bus.h), installed only
// around the Z80's own step; the MicroEngine side is the WordPort the core reads through.

#include "core/board.h"
#include "core/bus.h"
#include "cpu/cpu.h"
#include "cpu/cpuPmachine.h"
#include "cpu/cpuZ80.h"

#include <array>
#include <memory>

namespace altair {

class Pascal100Board : public Board, public BusMaster, public CpuCard {
public:
    Pascal100Board();

    std::string type() const override { return "pascal100"; }

    // The card is an IEEE 696 board: the Map drives A16 and the MicroEngine moves 16-bit
    // words (manual ch. II D, IX).

    // ---- BusMaster: the active core drives the bus; the other is held ----
    StepResult step(Bus& bus) override;

    // ---- CpuCard ----
    CpuCore* activeCore() override {
        return meActive_ ? static_cast<CpuCore*>(me_.get()) : static_cast<CpuCore*>(z80_.get());
    }
    std::vector<CoreUnit> cores() override { return {{"z80", z80_.get()}, {"me", me_.get()}}; }
    size_t coreCount() const override { return 2; }
    void settleSwitch() override;
    uint8_t peekCodeByte(CpuCore* core, const Bus& bus, uint32_t addr) override;
    void reportAchievedHz(long long hz) override { achievedHz_ = hz; }
    long long achievedHz() const override { return achievedHz_; }

    // ---- Board: the System Control Port and the Map Output Port (write-only) ----
    bool decodes(const BusCycle& c) const override;
    void write(const BusCycle& c) override;

    void reset(Reset r) override;
    void power() override;
    void clockAttached() override { publishPolicy(); }

    std::vector<Property> properties() override;
    std::vector<UnitDef> units() const override;
    std::vector<MapEntry> ioMap() const override;
    std::vector<std::string> drainLog() override;

    void serialize(StateWriter& w) const override;
    void deserialize(StateReader& r) override;

    // ---- For tests and the debugger ----
    CpuZ80* z80() { return z80_.get(); }
    PMachineCore* me() { return me_.get(); }
    bool meActive() const { return meActive_; }
    uint8_t latch() const { return u237_; }          // U237 outputs, bit n = output n
    uint8_t mapEntry(int i) const { return map_[i & 15]; }  // as stored (= bus A16-A13)
    bool meInterruptLatched() const { return meIntLatch_; }
    WordPort& mePort() { return meSide_; }   // the MicroEngine's view of memory and I/O

    // U237 outputs (manual ch. IV M; the low nibble of the Table II-1 values).
    enum Latch : uint8_t {
        kMeMap = 0,      // C0 / 40
        kZ80Map = 1,     // C1 / 41
        kProtectNmi = 2, // C2 / 42 -- Extended Map only; kept, no effect here
        kAutoXfer = 3,   // C3 / 43
        kMeInt = 4,      // C4 then 44
        kMapHalf = 5,    // C5 upper / 45 lower
    };

private:
    // The Z80's view: the Map, the mirror and jump-on-reset (core/bus.h AddressTranslator).
    struct Z80Side : AddressTranslator {
        Pascal100Board* card;
        explicit Z80Side(Pascal100Board* c) : card(c) {}
        uint32_t translate(uint16_t logical, Cycle) override;
        uint16_t ioAddress(uint16_t addr, Cycle) override;
        uint8_t readData(uint8_t v) override;
    };
    // The MicroEngine's view: words, the Map, memory-mapped I/O, its interrupt line.
    struct MeSide : WordPort {
        Pascal100Board* card;
        explicit MeSide(Pascal100Board* c) : card(c) {}
        uint16_t readWord(uint16_t w) override;
        void writeWord(uint16_t w, uint16_t v) override;
        bool interruptPending() override { return card->meIntLatch_; }
        uint16_t interruptVector() override;
    };

    bool latchBit(Latch b) const { return (u237_ >> b) & 1; }
    void writeControl(uint8_t v);              // System Control Port
    void writeMap(uint8_t a);                  // Map Output Port (Z80 only)
    uint32_t meByteToPhys(uint32_t byteAddr) const;
    void publishPolicy();
    void warnOnce(bool& said, const std::string& msg);

    std::unique_ptr<CpuZ80> z80_;
    std::unique_ptr<PMachineCore> me_;
    Z80Side z80Side_{this};
    MeSide meSide_{this};

    // State.
    bool meActive_ = false;            // U208B: 1 = the MicroEngine has control
    uint8_t u237_ = 0;                 // the addressable latch, bit n = output n
    std::array<uint8_t, 16> map_{};    // U216, as stored: the bus A16-A13 of each 8K page
    bool meIntLatch_ = false;          // the MicroEngine's interrupt request (C4 then 44)
    uint8_t jumpReads_ = 3;            // jump-on-reset reads done since reset (3 = off)

    // The Z80's own A8-A15 on its current I/O cycle, before the mirror: the Map port
    // takes its entry from here (B on an OUT (C),A; manual Fig. 2-2, ch. IV L).
    uint8_t z80IoHigh_ = 0;
    bool z80Io_ = false;               // the I/O cycle in flight is the Z80's
    bool meIo_ = false;                // the I/O cycle in flight is the MicroEngine's

    // Straps.
    uint8_t port_ = 0xD0;              // U225: the System Control Port (even)
    bool jumpOnReset_ = true;          // J202
    uint16_t resetAddr_ = 0x0000;      // U242: A15-A12 of the jump address
    long long clockHz_ = 0;            // the U102 header; 0 = flat out
    bool idle_ = true;
    long long achievedHz_ = 0;

    std::vector<std::string> log_;
    bool saidMeMap_ = false;
};

} // namespace altair
