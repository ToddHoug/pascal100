#pragma once
//
// PMachineCore -- the Western Digital Pascal MicroEngine, as a chip: a core that runs
// WD III.0 P-code the way WD's own description defines it.
//
// The specification is reference/80-0130016-00A0_..._Jul82.md: Table C-3 (section 2) for
// what each operator does, read with the settled readings of its section 7 (real code from
// the owner's Pascal-100 distribution disk settles what the printed table leaves unclear).
// Nothing here comes from another emulator.
//
// THE CHIP, NOT THE CARD (cpu.h). The MicroEngine is word-addressed: 64K 16-bit words,
// with byte operators that address byte 2w (low half) and 2w+1 (high half) of word w.
// On the Pascal-100 it reaches memory through the board's Map and the S-100 16-bit
// transfer logic (SIXTN), which belong to the board, not to the chip. So the core reads
// and writes memory through a WordPort that the card attaches, and step(Bus&) does not use
// the Bus it is handed. A test attaches a plain 64K-word memory.

#include "cpu/cpu.h"

#include <cstdint>
#include <string>

namespace altair {

namespace pcode { struct Decoded; }

// The MicroEngine's view of memory and of its interrupt line, supplied by the card.
struct WordPort {
    virtual ~WordPort() = default;
    virtual uint16_t readWord(uint16_t wordAddr) = 0;
    virtual void writeWord(uint16_t wordAddr, uint16_t value) = 0;

    // A byte address is 17 bits: byte 2w is the LOW half of word w (reference 7.4's
    // neighbour: the IEEE byte order, and C-3's `b[0]` = low byte, CPF's seg#/proc#).
    // A card with real byte cycles overrides these; the default goes through the word.
    virtual uint8_t readByte(uint32_t byteAddr) {
        uint16_t w = readWord((uint16_t)(byteAddr >> 1));
        return (byteAddr & 1) ? (uint8_t)(w >> 8) : (uint8_t)w;
    }
    virtual void writeByte(uint32_t byteAddr, uint8_t v) {
        uint16_t a = (uint16_t)(byteAddr >> 1);
        uint16_t w = readWord(a);
        w = (byteAddr & 1) ? (uint16_t)((w & 0x00FF) | (v << 8)) : (uint16_t)((w & 0xFF00) | v);
        writeWord(a, w);
    }

    // The interrupt the P-machine takes at Table C-3's label 6 (no task ready). The card
    // answers with the WORD address of the vector, whose contents point at the semaphore
    // to signal (`int_vec_address`, reference 2.21). Default: no interrupts.
    // The core calls interruptVector() once for each interrupt it takes: that call is the
    // acknowledge, after which interruptPending() answers for the next interrupt.
    virtual bool interruptPending() { return false; }
    virtual uint16_t interruptVector() { return 0; }
};

class PMachineCore : public CpuCore {
public:
    // Run-time error numbers that the P-machine raises (reference 7.9: the numbers the
    // owner's operating system prints, not the older list in the Pascal-100 manual B.1).
    enum Error : uint16_t {
        kRangeError = 1,
        kStackOverflow = 4,
        kDivByZero = 6,
        // "Value range error": the real Pascal-100, for 7 mod 0 and 7 mod -1 (reference 7.12).
        kModByNonPos = kRangeError,
        kFloatError = 12,
        kBreakpoint = 15,
    };

    // The word that holds the address of the boot block (Pascal-100 manual F 3.7.2).
    static constexpr uint16_t kInitVector = 0xFC68;
    static constexpr uint16_t kNil = 0xFC00;  // NIL = -1024 (C-33)

    void attach(WordPort* port) { port_ = port; }

    const char* isa() const override { return "pcode"; }
    // The longest C-1 operator with its parameters (LOD, STR, LDC, CXI...). Inline data
    // after it (LDC words, LSA strings) is not part of the disassembly. tests/test_pmachine.cpp
    // checks this against pcode::opTable().
    uint8_t maxInsnLen() const override { return 4; }
    std::vector<RegDef> registers() override;
    void captureRegs(std::vector<uint32_t>& out) override;  // registers()' values, fast
    void reset(Reset) override;
    StepResult step(Bus&) override;  // the Bus is not used: memory is the WordPort
    StepResult step();               // the same, for a caller with no Bus (tests)

    uint32_t pc() const override { return ipc_; }
    void setPc(uint32_t v) override { ipc_ = (uint16_t)v; }

    // Halted = idle at label 6 (no task ready, waiting for an interrupt) or stopped.
    bool halted() const override { return state_ != State::Running && state_ != State::Start; }
    bool interruptsEnabled() const override { return true; }
    // The MicroEngine's interrupt line is the WordPort's, not only the bus pINT.
    bool haltCanEnd(bool pint, bool eventsQueued) const override {
        return state_ == State::Idle &&
               (pint || eventsQueued || (port_ && port_->interruptPending()));
    }

    void serialize(StateWriter& w) const override;
    void deserialize(StateReader& r) override;

    // Why the core stopped, or empty while it runs. The core stops instead of guessing,
    // for example on an unassigned opcode (reference 6.10).
    bool stopped() const { return state_ == State::Stopped; }
    const std::string& stopReason() const { return stopReason_; }
    bool idle() const { return state_ == State::Idle; }
    // The M0015 time of the last step, tenths of a microsecond (0: no printed time).
    uint32_t lastTime10() const { return t10_; }

    // The registers (reference section 1, Pascal-100 manual F 3.7.2).
    uint16_t sp_ = 0, mp_ = 0, bp_ = 0, ipc_ = 0, segb_ = 0, ctp_ = 0, rqp_ = 0, sdp_ = 0;

private:
    enum class State : uint8_t {
        Start,    // after reset: the next step runs the boot sequence from kInitVector
        Running,
        Idle,     // label 6: no task is ready; waits for an interrupt
        Stopped,  // see stopReason_
    };

    // Memory.
    uint16_t rd(uint16_t a) { return port_->readWord(a); }
    void wr(uint16_t a, uint16_t v) { port_->writeWord(a, v); }
    void push(uint16_t v) { --sp_; wr(sp_, v); }
    uint16_t pop() { uint16_t v = rd(sp_); ++sp_; return v; }
    uint16_t tos() { return rd(sp_); }
    uint8_t codeByte(uint32_t at) { return port_->readByte(((uint32_t)segb_ * 2 + at) & 0x1FFFF); }

    // The pieces Table C-3 defines once and uses in many operators.
    uint16_t local(uint16_t off) const { return (uint16_t)(mp_ + 3 + off); }   // mp + mscw_sz - 1 + B
    uint16_t global(uint16_t off) const { return (uint16_t)(bp_ + 3 + off); }  // bp + mscw_sz - 1 + B
    uint16_t walkStatic(uint16_t from, int links);  // follow msstat `links` times
    uint16_t getsegb(uint16_t segno);               // segment base, through the SIB (2.6)
    uint16_t sibOf(uint16_t segno);
    uint8_t thisSegNo();                            // low byte of this segment's dictionary word

    // Reals (7.4): IEEE single precision, the high word (sign, exponent) at the LOWER
    // address, which is the top of the stack.
    float getReal(uint16_t a);
    void putReal(uint16_t a, float f);
    // An arithmetic result, or Raise(12) on overflow, underflow or division by zero
    // ("all over/underflows cause a run-time error", C-1). Returns false if it raised.
    bool realResult(float& r);
    void execSet(uint8_t op, int32_t ub);  // 2.14, 2.15
    void execByteCompare(uint8_t op, int32_t len);  // 2.16

    // The set and byte-array compares, read from the stack without a change. The operator
    // uses the result; the timing uses the counts (M0015's N, D and B/L/G).
    struct PwrScan { bool v; int32_t n, d; };
    PwrScan scanPwr(uint8_t op);     // EQUPWR, LEQPWR, GEQPWR
    struct ByteScan { int cmp; int32_t n; };
    ByteScan scanBytes(int32_t len); // EQUBYT, LEQBYT, GEQBYT: TOS-1 against TOS

    // M0015 timing (pmachineTiming.cpp), in tenths of a microsecond. time10() reads the
    // operands before the operator runs; a Raise adds the error call's time.
    uint32_t time10(const pcode::Decoded& d);
    uint32_t signalTime10(uint16_t sem);

    // 2.19: an external call through segment `segno`'s SIB, counted in its segrefs.
    void callExternal(uint16_t segno, uint16_t proc, uint16_t statLink);
    void addSegrefs(uint16_t segno, int delta);

    // 2.21: tasks. A queue is a list of TIBs linked through TIB word 0, highest priority
    // (TIB word 1, low byte) first.
    void updatetib();                                // save the live registers in the TIB
    void loadtib();                                  // load them from the TIB
    uint16_t enque(uint16_t head, uint16_t task);    // insert by priority, return the head
    void taskswitch();                               // label 5: save, then label 6
    void dispatch();                                 // label 6: run the next ready task
    void signal(uint16_t sem);                       // SIGNAL, also from an interrupt
    uint8_t prior(uint16_t tib) { return (uint8_t)rd((uint16_t)(tib + 1)); }
    void raise(uint16_t err);                       // Raise: push err, CXG 2,2
    // Call procedure `proc` of the segment at `newSegb` (7.2). `statLink` becomes msstat.
    // `segNoForMsseg` is stored in msseg (0 = same segment). Returns false if it raised.
    bool call(uint16_t newSegb, uint16_t proc, uint16_t statLink, uint8_t msseg);

    void stop(const std::string& why);
    void stopAt(uint16_t at, const std::string& why);  // inside an operator
    void boot();
    void execute();

    WordPort* port_ = nullptr;
    State state_ = State::Start;
    std::string stopReason_;
    bool raising_ = false;  // a Raise that itself overflows the stack stops the core
    uint32_t t10_ = 0;      // this step's time, tenths of a microsecond
    uint8_t carry10_ = 0;   // tenths not yet paid as a whole 2.5 MHz cycle (0..3)
};

} // namespace altair
