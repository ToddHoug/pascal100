#include "test.h"

#include "boards/compupro-interfacer3.h"
#include "config/toml.h"
#include "core/board.h"
#include "core/clock.h"
#include "core/machine.h"
#include "core/statefile.h"
#include "host/stream.h"

using namespace altair;

namespace {

// Two Interfacer 3 boards at 10H on a real bus: users 0-7 and 8-15, as MP/M 8-16's TTYS
// file expects. User 0 and user 9 have scripted lines.
const char* kTwoBoards = R"(
[machine]
name = "if3test"

[[board]]
type = "interfacer3"
id   = "if3a"

  [board.unit.user0]
  tx_int = "vi3"
  rx_int = "vi2"

[[board]]
type  = "interfacer3"
id    = "if3b"
users = "8-15"

  [board.unit.user1]
  rx_int = "vi2"
)";

struct Rig {
    Machine               m;
    InterfacerThreeBoard* a  = nullptr;
    InterfacerThreeBoard* b  = nullptr;
    ScriptedStream*       u0 = nullptr;
    ScriptedStream*       u9 = nullptr;

    Rig() {
        std::string err;
        CHECK(loadTomlText(kTwoBoards, "if3test", m, err), "a machine file with two interfacer3 boards loads");
        a = dynamic_cast<InterfacerThreeBoard*>(m.find("if3a"));
        b = dynamic_cast<InterfacerThreeBoard*>(m.find("if3b"));
        CHECK(a && b, "the registry makes both boards");
        if (!a || !b) return;
        CHECK(a->connect("user0", "scripted", err), "a scripted line on board A user 0");
        CHECK(b->connect("user1", "scripted", err), "a scripted line on board B user 1 (user 9)");
        u0 = dynamic_cast<ScriptedStream*>(a->unitStream("user0"));
        u9 = dynamic_cast<ScriptedStream*>(b->unitStream("user1"));
        if (!u0 || !u9) a = nullptr;  // every section stops at its `if (!g.a)` guard
        m.power();
    }
    uint8_t in(uint8_t p) { return m.bus.ioRead(p); }
    void    out(uint8_t p, uint8_t v) { m.bus.ioWrite(p, v); }
    // The standard async set-up of the manual's CP/M console sample: 8N2 9600, TxEN RxEN.
    void init(uint8_t user) {
        out(0x17, user);
        out(0x12, 0xEE);
        out(0x12, 0x7E);
        out(0x13, 0x27);
    }
    // As a polling guest does: look at the line, let time pass, look again. A frame starts
    // when a board polls its line, and RxRDY rises one character time later.
    void runFor(uint64_t t) {
        a->pump();
        b->pump();
        for (uint64_t i = 0; i < t; i += 1000) m.clock.advance(1000);
        a->pump();
        b->pump();
    }
};

} // namespace

void test_interfacer3() {
    SECTION("Interfacer 3 -- decode: the 8-port block, and only the selected board answers");
    {
        Rig g;
        if (!g.a) return;
        CHECK(g.a->selected() && !g.b->selected(), "after reset the latch is 0: board A answers");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x11, 0, false}).size() == 1,
              "one board answers a status read");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x17, 0, false}).empty(),
              "an IN from the user select port is not decoded");
        g.out(0x17, 0x09);  // user 9 = board B, user 1
        CHECK(g.a->selectLatch() == 0x09 && g.b->selectLatch() == 0x09,
              "BOTH boards latch the select write (U34 needs no SEL)");
        CHECK(!g.a->selected() && g.b->selected(), "now board B answers, board A is off the bus");
        CHECK(!g.m.bus.lastContended(), "and the select write was not a contention");
        g.out(0x16, 0x02);  // A0 is not decoded: port 16 is the select port too
        CHECK(g.a->selected() && g.a->selectLatch() == 0x02, "a write to relative port 6 selects");
        g.out(0x17, 0xE5);
        CHECK(g.a->selectLatch() == 0x05, "only D0-D4 are latched");
    }

    SECTION("Interfacer 3 -- a channel: program, send, receive, through the selected USART");
    {
        Rig g;
        if (!g.a) return;
        g.init(0);
        CHECK((g.in(0x11) & 0x01) == 0x01, "TxEN on: TxRDY (D0) reads 1");
        CHECK((g.in(0x11) & 0xC0) == 0xC0, "unconnected DSR and DCD read asserted (pulled up)");
        g.out(0x10, 'H');
        CHECK(g.u0 && g.u0->out() == "H", "the byte reaches user 0's line");
        g.u0->feed("k");
        g.runFor(100000);
        CHECK((g.in(0x11) & 0x02) == 0x02, "a typed key: RxRDY (D1)");
        CHECK(g.in(0x10) == 'k', "and the data port reads it");
        CHECK((g.in(0x11) & 0x02) == 0, "and the read clears RxRDY");
        g.init(9);
        g.out(0x10, 'Z');
        CHECK(g.u9 && g.u9->out() == "Z", "user 9 is board B's user 1");
        CHECK(g.u0->out() == "H", "and nothing more went to user 0");
    }

    SECTION("Interfacer 3 -- status ports 4/5 and the interrupt masks");
    {
        Rig g;
        if (!g.a) return;
        g.init(0);
        g.out(0x17, 0);
        CHECK(g.in(0x14) == 0x01, "port 4: only user 0's transmitter is enabled, so TxRDY0 alone");
        CHECK(g.m.bus.viLines() == 0, "masks clear after reset: no VI line");
        g.out(0x14, 0x01);
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi3), "tx mask bit 0: TxINT0 on VI3 (J15)");
        g.out(0x14, 0x00);
        g.u0->feed("x");
        g.runFor(100000);
        CHECK(g.in(0x15) == 0x01, "port 5 shows RxRDY0 with the rx mask clear");
        CHECK(g.m.bus.viLines() == 0, "but no VI line without the mask");
        g.out(0x15, 0x01);
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi2), "rx mask bit 0: RxINT0 on VI2 (J16)");
        g.in(0x10);
        CHECK(g.m.bus.viLines() == 0, "reading the byte drops RxINT0");
    }

    SECTION("Interfacer 3 -- an unselected board still drives its interrupts");
    {
        Rig g;
        if (!g.a) return;
        g.init(9);
        g.out(0x15, 0x02);  // board B: rx mask for its user 1
        g.out(0x17, 0x00);  // back to board A
        g.u9->feed("q");
        g.runFor(100000);
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi2),
              "a key on user 9 raises VI2 while board A is the one selected");
        g.out(0x17, 0x09);
        CHECK(g.in(0x15) == 0x02 && g.in(0x10) == 'q', "select it, read port 5, read the byte");
    }

    SECTION("Interfacer 3 -- reset, snapshot, CONFIG SAVE");
    {
        Rig g;
        if (!g.a) return;
        g.out(0x17, 0x0B);
        g.out(0x14, 0xFF);
        g.a->reset(Reset::Bus);
        g.b->reset(Reset::Bus);
        CHECK(g.a->selectLatch() == 0 && g.a->selected(), "reset clears the select latch");
        CHECK(g.m.bus.viLines() == 0, "and the masks");
        g.out(0x17, 0x0B);
        StateWriter w;
        g.b->serialize(w);
        Rig h;
        if (!h.b) return;
        StateReader r(w.data());
        h.b->deserialize(r);
        CHECK(h.b->selectLatch() == 0x0B && h.b->selected(), "a snapshot keeps the latch");
        const std::string out = saveTomlText(g.m);
        CHECK(out.find("users = \"8-15\"") != std::string::npos &&
                  out.find("tx_int = \"vi3\"") != std::string::npos,
              "CONFIG SAVE writes the board and unit settings back");
    }

    SECTION("Interfacer 3 -- the port must be a multiple of 8");
    {
        Rig g;
        if (!g.a) return;
        std::string err;
        CHECK(!setProperty(*g.a, "port", "0x14", err), "port 14H is refused");
        CHECK(setProperty(*g.a, "port", "0x18", err), "port 18H is taken");
    }
}
