#include "test.h"
#include "pcode_fixture.h"

#include "boards/digicomp-pascal100.h"
#include "boards/s100-memory.h"
#include "cli/monitor.h"
#include "core/debug.h"
#include "core/machine.h"
#include "core/statefile.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace altair;

// THE DIGICOMP PASCAL-100 (pascal100 Stage 4). Every expectation here is from the
// Pascal-100 User's Manual (Reference/pascal100/): ch. II B (System Control Port), II D
// (the Map), II E (I/O), II F (interrupts), IV I-P (the circuits).

namespace {

// An IEEE 696 machine with 128K of RAM (one board, 000000-01FFFF, like the real machine's
// RAM 23) and the card.
struct P100Rig {
    Machine m;
    Pascal100Board* card = nullptr;
    MemoryBoard* mem = nullptr;

    explicit P100Rig(bool sixteenBit = false) {
        std::string err;
        m.bus.setVerify(true);
        mem = dynamic_cast<MemoryBoard*>(m.add("memory", "mem0", err));
        // Every setting is CHECKed: a refused one would leave a different machine behind
        // and the tests would pass on it anyway.
        CHECK(setProperty(*mem, "ext", "true", err), ("mem0 ext = true: " + err).c_str());
        CHECK(setProperty(*mem, "fill", "zero", err), ("mem0 fill = zero: " + err).c_str());
        if (sixteenBit)
            CHECK(setProperty(*mem, "sixteen_bit", "true", err), ("mem0 sixteen_bit: " + err).c_str());
        Region r;
        r.kind = RegionKind::Ram;
        r.at = 0x000000;
        r.size = 0x20000;
        CHECK(mem->addRegion(r, err), ("mem0 128K at 000000: " + err).c_str());
        mem->power();
        card = dynamic_cast<Pascal100Board*>(m.add("pascal100", "p100", err));
        CHECK(card != nullptr, ("the machine takes a pascal100: " + err).c_str());
        if (card) card->power();
    }
    // Physical memory, as the monitor sees it (never through the Map).
    void load(uint32_t at, std::initializer_list<uint8_t> code) {
        for (uint8_t byte : code) m.bus.memWriteX(at++, byte);
    }
    uint8_t peek(uint32_t a) { return m.bus.peekX(a); }
    void scp(uint8_t v) { m.bus.ioWrite(0x00D0, v); }   // the monitor's OUT D0
    // Run until the active core halts (or `n` instructions).
    RunResult run(uint64_t n = 1000) { return m.debug.run(n); }
};

// Port 55: records the address (what the bus carried on A0-A15) and data of every write,
// and answers a read with 5A.
struct PortSpy : Board {
    std::string type() const override { return "test-portspy"; }
    bool decodes(const BusCycle& c) const override {
        return (c.type == Cycle::IoWrite || c.type == Cycle::IoRead) && (c.addr & 0xFF) == 0x55;
    }
    void write(const BusCycle& c) override {
        addrs.push_back(c.addr);
        data.push_back(c.data);
    }
    uint8_t read(const BusCycle& c) override {
        addrs.push_back(c.addr);
        return 0x5A;
    }
    std::vector<Property> properties() override { return {}; }
    std::vector<uint16_t> addrs;
    std::vector<uint8_t> data;
};

// Pulls pINT (pin 73) or NMI* (pin 12) on request.
struct WireBoard : Board {
    std::string type() const override { return "test-wires"; }
    bool assertsInt() const override { return pint; }
    bool assertsNmi() const override { return nmi; }
    void setInt(bool on) { pint = on; intChanged(); }
    void setNmi(bool on) { nmi = on; nmiChanged(); }
    std::vector<Property> properties() override { return {}; }
    bool pint = false, nmi = false;
};

// THE SOFT BOOT, IN MINIATURE (manual ch. V B; SBQB.ASM's TOMICRO/FROMMICRO).
//
// A P-machine system is built with the Stage 1 fixture (pcode_fixture.h: boot block at word
// F0, TIB at 100, segment 1 at 1000, frames at 5000..5800) and copied into physical memory
// -- all of it below 64K words, so in segment 0 -- EXCEPT the init vector at word FC68
// (byte 1F8D0), which the Z80 must write itself, through its Map, as the soft boot does.
//
// The MicroEngine's program (procedure 1 of segment 1):
//   LDCI 0050; LDCI 4D45; STO      word 50 (bytes A0/A1) := 4D45 -- the "answer"
//   LDCI FFD0; SLDC 0; STO         00 to the System Control Port: control to the Z80
//
// The Z80's program, from 0000:
//   0000 LD BC,20D1 ; LD A,C3 ; OUT (C),A   Map entry 1 = F: logical 2000-3FFF -> 1E000
//   0007 LD A,C1 ; OUT (D0),A               Map on for the Z80
//   000B LD HL,00F0 ; LD (38D0),HL          the init vector (word FC68 = byte 1F8D0) := F0
//   0011 LD A,41 ; OUT (D0),A               Map off again
//   0015 LD BC,FFD0 ; LD A,80 ; OUT (C),A   TOMICRO: B = FF, port D0 on A0-A7
//   001C LD A,(00A0) ; LD (00C0),A ; HALT   FROMMICRO: the answer
struct SoftBoot {
    P100Rig g;
    pfix::Sys sys;
    uint16_t ipcStart = 0, ipcEnd = 0, ipcSecondStore = 0;
    explicit SoftBoot(bool sixteen = false,
                      const char* prog = "LDCI 0x0050; LDCI 0x4D45; STO; LDCI 0xFFD0; SLDC 0; STO")
        : g(sixteen) {
        sys.prog(prog);
        ipcStart = sys.seg1.codeStart[1];
        ipcEnd = sys.seg1.codeEnd[1];
        ipcSecondStore = (uint16_t)(sys.seg1.codeStart[1] + 7);   // after LDCI LDCI STO
        for (uint32_t w = 0; w < 0x10000; ++w) {
            uint16_t v = sys.mem.m[w];
            if (v == 0 || w == PMachineCore::kInitVector) continue;
            g.load(w * 2, {(uint8_t)v, (uint8_t)(v >> 8)});
        }
        g.load(0x0000, {0x01, 0xD1, 0x20, 0x3E, 0xC3, 0xED, 0x79, 0x3E, 0xC1, 0xD3, 0xD0,
                        0x21, 0xF0, 0x00, 0x22, 0xD0, 0x38, 0x3E, 0x41, 0xD3, 0xD0,
                        0x01, 0xD0, 0xFF, 0x3E, 0x80, 0xED, 0x79,
                        0x3A, 0xA0, 0x00, 0x32, 0xC0, 0x00, 0x76});
    }
};

template <class B> B* adopt(Machine& m, const char* id) {
    auto* b = new B;
    b->id = id;
    m.adopt(std::unique_ptr<Board>(b));
    return b;
}

} // namespace

void test_pascal100() {
    SECTION("pascal100 -- reset state (ch. II B)");
    {
        P100Rig g;
        CHECK(g.card->latch() == 0, "every U237 option is off");
        CHECK(!g.card->meActive() && g.m.cpu() == g.card->z80(), "the Z80 has control");
        g.scp(0x80);
        g.scp(0xC3);
        g.scp(0xC5);
        g.card->reset(Reset::Bus);
        CHECK(g.card->latch() == 0 && !g.card->meActive(),
              "a reset puts the Z80 back in control and clears every option");
    }

    SECTION("pascal100 -- every Table II-1 value");
    {
        P100Rig g;
        struct Row { uint8_t v; int bit; bool on; const char* what; };
        const Row rows[] = {
            {0xC0, Pascal100Board::kMeMap, true, "C0: Map on for the MicroEngine"},
            {0x40, Pascal100Board::kMeMap, false, "40: Map off for the MicroEngine"},
            {0xC1, Pascal100Board::kZ80Map, true, "C1: Map on for the Z80"},
            {0x41, Pascal100Board::kZ80Map, false, "41: Map off for the Z80"},
            {0xC2, Pascal100Board::kProtectNmi, true, "C2: NMI on protect"},
            {0x42, Pascal100Board::kProtectNmi, false, "42: no NMI on protect"},
            {0xC3, Pascal100Board::kAutoXfer, true, "C3: auto-transfer on"},
            {0x43, Pascal100Board::kAutoXfer, false, "43: auto-transfer off"},
            {0xC5, Pascal100Board::kMapHalf, true, "C5: upper half of the Map"},
            {0x45, Pascal100Board::kMapHalf, false, "45: lower half of the Map"},
        };
        for (const Row& r : rows) {
            g.scp(r.v);
            CHECK((((g.card->latch() >> r.bit) & 1) != 0) == r.on, r.what);
        }
        CHECK(g.card->latch() == 0, "each option is its own bit: all off again");

        g.scp(0x80);
        CHECK(g.card->meActive() && g.m.cpu() == g.card->me(), "80: the MicroEngine");
        g.scp(0x00);
        CHECK(!g.card->meActive() && g.m.cpu() == g.card->z80(), "00: the Z80");

        g.scp(0xC4);
        g.scp(0x44);
        CHECK(g.card->meInterruptLatched() && ((g.card->latch() >> 4) & 1) == 0,
              "C4 then 44: the line is down again, the MicroEngine's request is held");
    }

    SECTION("pascal100 -- the port is decoded by bits (ch. IV M)");
    {
        P100Rig g;
        g.scp(0x81);
        CHECK(g.card->meActive(), "81: DO6 = 0, DO7 = 1 -- U208B selects the MicroEngine");
        g.scp(0x3F);
        CHECK(!g.card->meActive() && g.card->latch() == 0,
              "3F: DO6 = 0, DO7 = 0 -- the Z80, and U237 is untouched");
        g.scp(0xC8 | 0x01);
        CHECK(((g.card->latch() >> 1) & 1) == 1, "C9: DO3 is not an address bit -- output 1");
        g.m.bus.ioWrite(0x12D0, 0xC0);
        CHECK(g.card->latch() & 1, "A8-A15 are not decoded (QB's OUT (C),A has B anywhere)");
        g.m.bus.ioWrite(0x00D2, 0x80);
        CHECK(!g.card->meActive(), "D2 is not the card's port");
        CHECK(g.m.bus.ioRead(0x00D0) == 0xFF, "the port is write-only: nothing answers a read");
    }

    SECTION("pascal100 -- straps");
    {
        P100Rig g;
        std::string err;
        CHECK(!setProperty(*g.card, "port", "D1", err), "port must be even");
        CHECK(setProperty(*g.card, "port", "40", err), "port = 40");
        g.m.bus.ioWrite(0x0040, 0x80);
        CHECK(g.card->meActive(), "the System Control Port follows U225");
        CHECK(!setProperty(*g.card, "reset_addr", "1234", err), "reset_addr is a 4K boundary");
        CHECK(setProperty(*g.card, "reset_addr", "F000", err), "reset_addr = F000");
    }

    SECTION("pascal100 -- units and cores");
    {
        P100Rig g;
        auto u = g.card->units();
        CHECK(u.size() == 2 && u[0].name == "z80" && u[1].name == "me", "units z80 and me");
        CHECK(u[0].state == "active" && u[1].state == "(idle)", "the Z80 is the active one");
        CHECK(g.m.coreByRef("p100:me") == g.card->me(), "p100:me names the MicroEngine");
        CHECK(g.card->me()->isa() == std::string("pcode") &&
                  g.card->z80()->isa() == std::string("z80"),
              "each core speaks its own ISA");
    }

    // ---- Task 2: the Map, the mirror, jump-on-reset ----
    //
    // The Z80 programs start at 0000: J202 is on by default, so the first three reads
    // after reset are C3 00 00 (JP 0000) and the code at 0000 then runs as it is.

    SECTION("pascal100 -- loading the Standard Map (ch. II D.3, Fig. 2-2)");
    {
        P100Rig g;
        // 0000 LD A,45 ; OUT (D0),A    lower half
        // 0004 LD BC,20D1              B = 001xxxxx: entry 1
        // 0007 LD A,C3 ; OUT (C),A     A bits 5-2 = 0000 -> stored inverted = 1111
        // 000B LD A,C5 ; OUT (D0),A    upper half
        // 000F LD BC,E0D1              B = 111xxxxx: entry 8+7 = 15
        // 0012 LD A,FB ; OUT (C),A     A bits 5-2 = 1110 -> stored 0001
        // 0016 HALT
        g.load(0x0000, {0x3E, 0x45, 0xD3, 0xD0, 0x01, 0xD1, 0x20, 0x3E, 0xC3, 0xED, 0x79,
                        0x3E, 0xC5, 0xD3, 0xD0, 0x01, 0xD1, 0xE0, 0x3E, 0xFB, 0xED, 0x79,
                        0x76});
        RunResult r = g.run();
        CHECK(r.why == StopReason::Halted, "the loader halts");
        CHECK(g.card->mapEntry(1) == 0x0F, "entry 1 from B's top bits, A bits 5-2 inverted");
        CHECK(g.card->mapEntry(15) == 0x01, "entry 15: the half-select bit is A16 of the entry");
        bool others = true;
        for (int i = 0; i < 16; ++i)
            if (i != 1 && i != 15 && g.card->mapEntry(i) != 0) others = false;
        CHECK(others, "no other entry changed");
        CHECK(g.card->z80()->pc() == 0x0017, "the Z80 ran through (the map is off for it)");
    }

    SECTION("pascal100 -- the Z80 through its Map: 38D0 reaches 1F8D0 (the soft boot)");
    {
        P100Rig g;
        g.load(0x1F8D0, {0xA5});
        g.load(0x038D0, {0x11});   // what an unmapped read would see
        // 0000 LD BC,20D1 ; LD A,C3 ; OUT (C),A    entry 1 = F: logical 2000-3FFF -> 1E000
        // 0007 LD A,(38D0) ; LD (0101),A           entry loaded, Map still off
        // 000D LD A,C1 ; OUT (D0),A                Map on for the Z80
        // 0011 LD A,(38D0) ; LD (0100),A ; HALT    entry 0 = 0 keeps the code where it is
        g.load(0x0000, {0x01, 0xD1, 0x20, 0x3E, 0xC3, 0xED, 0x79, 0x3A, 0xD0, 0x38, 0x32,
                        0x01, 0x01, 0x3E, 0xC1, 0xD3, 0xD0, 0x3A, 0xD0, 0x38, 0x32, 0x00,
                        0x01, 0x76});
        RunResult r = g.run();
        CHECK(r.why == StopReason::Halted, "the program halts");
        CHECK(g.peek(0x0101) == 0x11, "with the Map off, 38D0 is physical 38D0");
        CHECK(g.peek(0x0100) == 0xA5, "with it on, LD A,(38D0) read physical 1F8D0");
        g.scp(0x41);
        CHECK(!(g.card->latch() & 2), "41: the Map is off for the Z80 again");
    }

    SECTION("pascal100 -- the address mirror (ch. IV K)");
    {
        P100Rig g;
        auto* spy = new PortSpy;
        spy->id = "spy";
        g.m.adopt(std::unique_ptr<Board>(spy));
        // 0000 LD BC,1255 ; LD A,77 ; OUT (C),A ; OUT (55),A ; HALT
        g.load(0x0000, {0x01, 0x55, 0x12, 0x3E, 0x77, 0xED, 0x79, 0xD3, 0x55, 0x76});
        g.run();
        CHECK(spy->addrs.size() == 2 && spy->addrs[0] == 0x5555 && spy->addrs[1] == 0x5555,
              "both OUTs put 55 on A8-A15: B (12) and A (77) never reach the bus");
    }

    SECTION("pascal100 -- jump-on-reset (ch. IV P)");
    {
        P100Rig g;
        std::string err;
        setProperty(*g.card, "reset_addr", "F000", err);
        g.load(0xF000, {0x76});
        g.card->reset(Reset::Bus);
        g.run(1);
        CHECK(g.card->z80()->pc() == 0xF000, "J202 on: the first instruction is JP F000");
        CHECK(g.peek(0x0000) == 0x00, "memory at 0000 is untouched (the data was replaced)");
        g.run(1);
        CHECK(g.card->z80()->pc() == 0xF001, "and the jump is spent: F000 runs normally");

        setProperty(*g.card, "jump_on_reset", "false", err);
        g.card->reset(Reset::Bus);
        g.run(1);
        CHECK(g.card->z80()->pc() == 0x0001, "J202 off: the Z80 runs the NOP at 0000");
    }
    {
        // The cycles still go out (U230 is disabled, not the bus cycle).
        P100Rig g;
        g.card->reset(Reset::Bus);
        int bp = g.m.debug.add(BreakKind::MemRead, 0x0000, 0x0000);
        RunResult r = g.run(1);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "a MEM R 0000 breakpoint fires");
        r = g.run(1);
        CHECK(g.card->z80()->pc() == 0x0000, "and after it the JP 0000 still runs");
    }

    SECTION("pascal100 -- a Map write that is not the Z80's (the monitor's OUT D1)");
    {
        P100Rig g;
        g.m.bus.ioWrite(0x40D1, 0xFF);   // A8-A15 = 40: entry 2; A bits 5-2 = 1111 -> 0000
        g.m.bus.ioWrite(0x60D1, 0xC3);   // entry 3 = 1111
        CHECK(g.card->mapEntry(2) == 0x00 && g.card->mapEntry(3) == 0x0F,
              "the entry comes from the bus's own A8-A15");
    }

    // ---- Task 3: the MicroEngine's view, interrupts, switching by wire ----

    SECTION("pascal100 -- MicroEngine words: byte 2w is the low half (ch. II C, IX)");
    for (bool sixteen : {false, true}) {
        P100Rig g(sixteen);
        const std::string mem = sixteen ? " (16-bit memory, SIXTN)" : " (8-bit memory, two cycles)";
        WordPort& me = g.card->mePort();
        g.load(0x1F8D0, {0x34, 0x12});
        CHECK(me.readWord(0xFC68) == 0x1234, ("word FC68 is bytes 1F8D0/1F8D1" + mem).c_str());
        me.writeWord(0x0040, 0xABCD);
        CHECK(g.peek(0x00080) == 0xCD && g.peek(0x00081) == 0xAB,
              ("a word write: even byte low, odd byte high" + mem).c_str());
    }

    SECTION("pascal100 -- the MicroEngine through its Map: all 16 entries");
    {
        P100Rig g;
        WordPort& me = g.card->mePort();
        g.load(0x038D0, {0x78, 0x56});
        g.m.bus.ioWrite(0x00D0, 0xC5);   // upper half
        g.m.bus.ioWrite(0xE0D1, 0xFB);   // entry 15 = 0001: byte 1E000-1FFFF -> 02000-03FFF
        CHECK(me.readWord(0xFC68) == 0x0000, "Map off: word FC68 is still physical 1F8D0");
        g.scp(0xC0);
        CHECK(me.readWord(0xFC68) == 0x5678, "C0: word FC68 (byte 1F8D0, entry 15) -> 038D0");
        CHECK(!(g.card->latch() & 2), "the Z80's Map stays off");
    }

    SECTION("pascal100 -- MicroEngine I/O: words FFpp and FEpp (ch. II E.2)");
    {
        P100Rig g;
        auto* spy = adopt<PortSpy>(g.m, "spy");
        WordPort& me = g.card->mePort();
        me.writeWord(0xFF55, 0x1234);
        CHECK(spy->addrs.size() == 1 && spy->addrs[0] == 0x5555 && spy->data[0] == 0x34,
              "FF55: one byte, the low half, to port 55 on both address halves");
        CHECK(me.readWord(0xFF55) == 0x5A5A, "a read: the byte in both halves (ch. IV I.1)");
        CHECK(spy->addrs.size() == 2 && spy->addrs[1] == 0x5555,
              "a read also has port 55 on both address halves");
        CHECK(me.readWord(0xFE55) == 0x5A5A && spy->addrs.size() == 3,
              "FE55: no SIXTN on I/O, so one byte, the same cycle");
        g.scp(0xC0);
        CHECK(me.readWord(0xFF55) == 0x5A5A, "the Map does not apply to the I/O window");
        CHECK(g.peek(0x1FEAA) == 0x00, "and no memory cycle went to byte 1FEAA");
    }

    SECTION("pascal100 -- the MicroEngine gives control back by writing word FFD0");
    {
        P100Rig g;
        g.scp(0x80);
        g.card->mePort().writeWord(0xFFD0, 0x0000);
        CHECK(!g.card->meActive(), "00 to the System Control Port: the Z80 has control");
        g.card->mePort().writeWord(0xFFD1, 0x0000);
        auto log = g.card->drainLog();
        CHECK(log.size() == 1 && log[0].find("only the Z80 can load the Map") != std::string::npos,
              "a MicroEngine write to the Map port is ignored, and said once");
        CHECK(g.card->mapEntry(0) == 0, "the Map did not change");
    }

    SECTION("pascal100 -- interrupting the MicroEngine (ch. II F.3)");
    {
        P100Rig g;
        WordPort& me = g.card->mePort();
        g.load(0x1F880, {0x11, 0x22});
        CHECK(!me.interruptPending(), "no request at reset");
        g.scp(0xC4);
        g.scp(0x44);
        CHECK(me.interruptPending(), "C4 then 44: a request the MicroEngine will see");
        CHECK(me.interruptVector() == 0xFC60, "the vector is at word FC60 (byte 1F8C0)");
        CHECK(g.peek(0x1F880) == 0xFF && g.peek(0x1F881) == 0xFF,
              "the acknowledge wrote word FC40 (bytes 1F880/1F881)");
        CHECK(!me.interruptPending(), "and the request is spent");
        g.scp(0x44);
        CHECK(!me.interruptPending(), "44 alone requests nothing");
    }

    SECTION("pascal100 -- pINT and NMI take control from the MicroEngine (ch. IV M, O)");
    {
        P100Rig g;
        auto* w = adopt<WireBoard>(g.m, "wires");
        g.scp(0x80);
        w->setInt(true);
        g.card->settleSwitch();
        CHECK(g.card->meActive(), "pINT with auto-transfer off: the MicroEngine keeps control");
        g.scp(0xC3);
        g.card->settleSwitch();
        CHECK(!g.card->meActive(), "C3: pINT resets U208B -- the Z80");
        g.scp(0x80);
        g.card->settleSwitch();
        CHECK(!g.card->meActive(), "while pINT is up the MicroEngine cannot keep control");
        w->setInt(false);
        g.scp(0x43);
        g.scp(0x80);
        w->setNmi(true);
        g.card->settleSwitch();
        CHECK(!g.card->meActive(), "NMI resets U208B whatever auto-transfer says");
    }

    // ---- Task 4: the gate -- the soft boot's hand-off, start to finish ----

    SECTION("pascal100 -- gate: the Z80 maps 1F8D0, starts the MicroEngine, gets control back");
    for (bool sixteen : {false, true}) {
        SoftBoot b(sixteen);
        P100Rig& g = b.g;
        const std::string mem = sixteen ? " (16-bit memory)" : " (8-bit memory)";
        CHECK(g.peek(0x1F8D0) == 0 && g.peek(0x1F8D1) == 0, "word FC68 starts empty");
        RunResult r = g.run(500);
        CHECK(r.why == StopReason::Halted, ("the Z80 halts at the end" + mem).c_str());
        CHECK(g.peek(0x1F8D0) == 0xF0 && g.peek(0x1F8D1) == 0x00,
              ("the Z80 wrote the init vector at 1F8D0 through its Map" + mem).c_str());
        CHECK(g.card->me()->ctp_ == pfix::Sys::TIB && g.card->me()->sdp_ == pfix::Sys::SSV,
              ("the MicroEngine booted from the block FC68 points at" + mem).c_str());
        CHECK(g.card->me()->ipc_ == b.ipcEnd && !g.card->me()->stopped(),
              ("and ran its program to the end" + mem).c_str());
        CHECK(g.peek(0x00C0) == 0x45, ("the Z80 read the MicroEngine's answer" + mem).c_str());
        CHECK(!g.card->meActive() && g.card->z80()->pc() == 0x0023,
              ("the Z80 resumed after its OUT and ran to its HALT" + mem).c_str());
    }

    SECTION("pascal100 -- gate: BREAK p100:me stops inside the MicroEngine's program");
    {
        SoftBoot b;
        P100Rig& g = b.g;
        int bp = g.m.debug.add(BreakKind::Pc, b.ipcSecondStore, b.ipcSecondStore);
        CHECK(g.m.debug.setCore(bp, "p100:me"), "bound to the MicroEngine");
        RunResult r = g.run(500);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "it stops on the breakpoint");
        CHECK(g.card->meActive() && r.pc == b.ipcSecondStore, "with the MicroEngine at that IPC");
        CHECK(g.peek(0x00A0) == 0x45 && g.peek(0x00C0) == 0x00,
              "after its first store, before the Z80 got control back");

        // SNAPSHOT/RESTORE in the middle of the MicroEngine's run.
        std::string path = (std::filesystem::temp_directory_path() / "pascal100-gate.snp").string();
        std::string err;
        std::vector<std::string> notes;
        CHECK(g.m.snapshot(path, err), ("snapshot with the MicroEngine active: " + err).c_str());
        SoftBoot c;
        CHECK(c.g.m.restore(path, err, notes), ("restored: " + err).c_str());
        CHECK(c.g.card->meActive() && c.g.card->me()->ipc_ == b.ipcSecondStore,
              "the restored machine has the MicroEngine active at the same IPC");
        r = c.g.run(500);
        CHECK(r.why == StopReason::Halted && c.g.peek(0x00C0) == 0x45 &&
                  c.g.card->z80()->pc() == 0x0023,
              "and runs on to the same end");
        std::filesystem::remove(path);
    }

    // ---- Task 5: the debugger on two cores ----

    SECTION("pascal100 -- history and REGS decode each core's own code");
    {
        SoftBoot b;
        P100Rig& g = b.g;
        g.m.debug.clearInsnHistory();
        g.run(500);
        auto recs = g.m.debug.insnHistory(100);
        // The MicroEngine's first record is its boot; the one at the program's first IPC
        // must hold the program's first operator.
        const Debugger::InsnRec* first = nullptr;
        for (const auto& rec : recs)
            if (rec.core == g.card->me() && rec.pc == b.ipcStart) { first = &rec; break; }
        CHECK(first && first->bytes[0] == 0x81 && first->bytes[1] == 0x50 &&
                  first->bytes[2] == 0x00,
              "the record at the first IPC holds the MicroEngine's code: LDCI 0050 (81 50 00)");

        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("HISTORY CPU 40", o);
        CHECK(o.str().find("[me]") != std::string::npos &&
                  o.str().find("LDCI 0050") != std::string::npos,
              "HISTORY CPU shows the MicroEngine's operators by name");
        CHECK(o.str().find("[z80]") != std::string::npos, "and the Z80's lines are tagged too");
    }
    {
        SoftBoot b;
        P100Rig& g = b.g;
        int bp = g.m.debug.add(BreakKind::Pc, b.ipcSecondStore, b.ipcSecondStore);
        g.m.debug.setCore(bp, "p100:me");
        g.run(500);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("REGS", o);
        CHECK(o.str().rfind("[me] ", 0) == 0 && o.str().find("LDCI FFD0") != std::string::npos,
              ("REGS on the MicroEngine shows the operator at SEGB*2+IPC: " + o.str()).c_str());
    }

    SECTION("pascal100 -- NEXT on the MicroEngine is a STEP");
    {
        // FJP is D4 -- the 8080's CNC. Taken, it never reaches IPC+2, so an 8080-style
        // step-over would run on past it; a STEP stops at the jump's target.
        //   +0 SLDC 0 ; +1 FJP 1 ; +3 NOP ; +4 LDCI FFD0 ; SLDC 0 ; STO
        SoftBoot b(false, "SLDC 0; FJP 1; NOP; LDCI 0xFFD0; SLDC 0; STO");
        P100Rig& g = b.g;
        uint16_t fjp = (uint16_t)(b.ipcStart + 1);
        int bp = g.m.debug.add(BreakKind::Pc, fjp, fjp);
        g.m.debug.setCore(bp, "p100:me");
        RunResult r = g.run(500);
        CHECK(r.why == StopReason::Breakpoint && g.card->me()->ipc_ == fjp, "stopped at FJP");
        std::string err;
        g.m.debug.remove(bp, err);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("NEXT", o);
        CHECK(g.card->meActive() && g.card->me()->ipc_ == b.ipcStart + 4,
              "NEXT ran the one operator: the jump was taken, the MicroEngine is at its target");
    }

    SECTION("pascal100 -- RESET CPU resets the active processor only, and says so");
    {
        P100Rig g;
        g.card->z80()->setPc(0x1234);
        g.scp(0x80);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("RESET CPU", o);
        CHECK(o.str().find("me only") != std::string::npos, ("names the core: " + o.str()).c_str());
        CHECK(g.card->meActive() && g.card->z80()->pc() == 0x1234,
              "the select latch and the Z80 are untouched");
        g.scp(0x00);
        std::ostringstream o2;
        mon.exec("RESET CPU", o2);
        CHECK(o2.str().find("z80 only, PC=0000") != std::string::npos &&
                  g.card->z80()->pc() == 0x0000,
              ("on the Z80: " + o2.str()).c_str());
    }

    // ---- Fixes from the /code-review high pass ----

    SECTION("pascal100 -- a cycle breakpoint on the 2nd jump-on-reset read does not spend it");
    {
        P100Rig g;
        std::string err;
        setProperty(*g.card, "reset_addr", "F000", err);
        g.load(0xF000, {0x76});
        g.card->reset(Reset::Bus);
        int bp = g.m.debug.add(BreakKind::MemRead, 0x0001, 0x0001);
        RunResult r = g.run(1);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "MEM R 0001 stops the first JP");
        std::string e2;
        g.m.debug.remove(bp, e2);
        g.run(1);
        CHECK(g.card->z80()->pc() == 0xF000, "the JP runs again from its first byte: PC = F000");
    }

    SECTION("pascal100 -- right after a reset, REGS and NEXT see the jump-on-reset bytes");
    {
        P100Rig g;
        std::string err;
        setProperty(*g.card, "reset_addr", "F000", err);
        g.load(0x0000, {0xCD, 0x00, 0x80});   // CALL 8000 in memory -- not what the Z80 runs
        g.load(0xF000, {0x76});
        g.card->reset(Reset::Bus);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("REGS", o);
        CHECK(o.str().find("JP") != std::string::npos && o.str().find("F000") != std::string::npos,
              ("REGS shows JP F000: " + o.str()).c_str());
        std::ostringstream o2;
        mon.exec("NEXT", o2);
        CHECK(g.card->z80()->pc() == 0xF000, "NEXT is one step to F000, not a run to 0003");
    }

    SECTION("pascal100 -- a cycle breakpoint on the FC40 write does not lose the interrupt");
    {
        // The MicroEngine WAITs on a semaphore with no task ready: it is idle at label 6.
        // The interrupt's vector (word FC60) points at that semaphore, so SIGNAL wakes the
        // task, which then gives control back to the Z80.
        SoftBoot b(false, "LAO 1; WAIT; LDCI 0xFFD0; SLDC 0; STO");
        P100Rig& g = b.g;
        const uint16_t sem = (uint16_t)(pfix::Sys::GLOBALS + 3 + 1);
        g.load((uint32_t)sem * 2, {0x00, 0x00, 0x00, 0xFC});   // count 0, waitq NIL (FC00)
        g.load(0x1F8C0, {(uint8_t)(sem & 0xFF), (uint8_t)(sem >> 8)});  // FC60 -> the semaphore
        RunResult r = g.run(500);
        CHECK(g.card->meActive() && g.card->me()->idle(), "the MicroEngine is idle at label 6");
        g.scp(0xC4);
        g.scp(0x44);
        int bp = g.m.debug.add(BreakKind::MemWrite, 0x1F880, 0x1F880);
        r = g.run(500);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "MEM W 1F880 stops the acknowledge");
        r = g.run(500);
        CHECK(r.why == StopReason::Halted && !g.card->meActive() && g.peek(0x00C0) == 0x00 &&
                  g.card->z80()->pc() == 0x0023,
              "resumed, the interrupt is still taken: the task runs and the Z80 halts at its end");
    }

    SECTION("pascal100 -- the bus map lists both ports");
    {
        P100Rig g;
        auto io = g.card->ioMap();
        CHECK(io.size() == 2 && io[0].lo == 0xD0 && io[1].lo == 0xD1, "D0 and D1");
    }

    SECTION("pascal100 -- snapshot keeps both cores and every latch");
    {
        P100Rig g;
        g.scp(0xC3);
        g.scp(0xC4);
        g.scp(0x80);
        g.card->z80()->setPc(0x1234);
        StateWriter w;
        g.card->serialize(w);

        P100Rig h;
        StateReader r(w.data());
        h.card->deserialize(r);
        CHECK(h.card->meActive() && h.card->latch() == g.card->latch() &&
                  h.card->meInterruptLatched(),
              "U208B, U237 and the MicroEngine's request come back");
        CHECK(h.card->z80()->pc() == 0x1234, "the held Z80 comes back where it was");
    }
}
