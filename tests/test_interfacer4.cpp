#include "test.h"

#include "boards/compupro-interfacer3.h"
#include "boards/compupro-interfacer4.h"
#include "config/toml.h"
#include "core/board.h"
#include "core/clock.h"
#include "core/machine.h"
#include "core/statefile.h"
#include "host/stream.h"

using namespace altair;

namespace {

// A line that remembers the title the machine hands it (ByteStream::setTitle).
struct TitleSpy : NullStream {
    std::string* seen;
    explicit TitleSpy(std::string* s) : seen(s) {}
    void setTitle(const std::string& t) override { *seen = t; }
};

// One group of eight users at 10H made of two Interfacer 4s (users 0-3 and 4-7, the CompuPro
// standard half), and an Interfacer 3 at the same base for users 8-15 (page 7: the two
// boards intermix at the same port addresses).
const char* kGroup = R"(
[machine]
name = "if4test"

[[board]]
type  = "interfacer4"
id    = "lo"
users = "0-3"

  [board.unit.user1]
  tx_int = "vi3"
  rx_int = "vi2"

[[board]]
type  = "interfacer4"
id    = "hi"
users = "4-7"

  [board.unit.centronics]
  tx_int = "vi5"

  [board.unit.parallel]
  rx_int = "vi6"

[[board]]
type  = "interfacer3"
id    = "if3"
users = "8-15"
)";

struct Rig {
    Machine              m;
    InterfacerFourBoard* lo  = nullptr;
    InterfacerFourBoard* hi  = nullptr;
    InterfacerThreeBoard* i3 = nullptr;
    ScriptedStream*      u1  = nullptr;  // lo user1 = exact user 1
    ScriptedStream*      lpt = nullptr;  // hi centronics = exact user 4
    ScriptedStream*      par = nullptr;  // hi parallel

    explicit Rig(const char* text = kGroup) {
        std::string err;
        CHECK(loadTomlText(text, "if4test", m, err), "a machine file with interfacer4 boards loads");
        lo = dynamic_cast<InterfacerFourBoard*>(m.find("lo"));
        hi = dynamic_cast<InterfacerFourBoard*>(m.find("hi"));
        i3 = dynamic_cast<InterfacerThreeBoard*>(m.find("if3"));
        CHECK(hi != nullptr, "the registry makes an interfacer4");
        if (!hi) return;
        if (lo) {
            CHECK(lo->connect("user1", "scripted", err), "a scripted line on lo user1");
            u1 = dynamic_cast<ScriptedStream*>(lo->unitStream("user1"));
        }
        CHECK(hi->connect("centronics", "scripted", err), "a scripted printer on hi");
        CHECK(hi->connect("parallel", "scripted", err), "a scripted device on hi's parallel port");
        lpt = dynamic_cast<ScriptedStream*>(hi->unitStream("centronics"));
        par = dynamic_cast<ScriptedStream*>(hi->unitStream("parallel"));
        if (!lpt || !par || (lo && !u1)) hi = nullptr;  // every section stops at its guard
        m.power();
    }
    uint8_t in(uint8_t p) { return m.bus.ioRead(p); }
    void    out(uint8_t p, uint8_t v) { m.bus.ioWrite(p, v); }
    void    init(uint8_t user) {  // the manual's CP/M console set-up (page 33)
        out(0x17, user);
        out(0x12, 0xEE);
        out(0x12, 0x7E);
        out(0x13, 0x27);
    }
    void runFor(uint64_t t) {
        for (Board* b : {(Board*)lo, (Board*)hi, (Board*)i3})
            if (b) b->pump();
        for (uint64_t i = 0; i < t; i += 1000) m.clock.advance(1000);
        for (Board* b : {(Board*)lo, (Board*)hi, (Board*)i3})
            if (b) b->pump();
    }
};

} // namespace

void test_interfacer4() {
    SECTION("Interfacer 4 -- decode: half a group, and the shared select latch");
    {
        Rig g;
        if (!g.hi) return;
        CHECK(g.lo->selected() && !g.hi->selected(), "after reset the latch is 0: the 0-3 board");
        g.out(0x17, 0x05);
        CHECK(g.lo->selectLatch() == 5 && g.hi->selectLatch() == 5, "every board latches the select");
        CHECK(!g.lo->selected() && g.hi->selected(), "user 5 is the 4-7 board");
        CHECK(!g.m.bus.lastContended(), "and the select write was not a contention");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x11, 0, false}).size() == 1,
              "ports 0-3: only the board of the selected half answers");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x14, 0, false}).size() == 2,
              "ports 4/5: both boards of the group answer (USEL has no H/L term)");
        g.out(0x16, 0x09);  // A0 is not decoded
        CHECK(g.hi->selectLatch() == 9 && !g.hi->selected() && !g.lo->selected(),
              "port 16 selects too: user 9, the Interfacer 3's group");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x14, 0, false}).size() == 1,
              "and the Interfacer 3 alone answers port 14");
        CHECK(g.m.bus.respondersTo(BusCycle{Cycle::IoRead, 0x17, 0, false}).empty(),
              "an IN from the select port is not decoded");
    }

    SECTION("Interfacer 4 -- ports 4/5: one nibble per board, no contention");
    {
        Rig g;
        if (!g.hi) return;
        g.init(1);  // lo user1: TxEN
        g.out(0x17, 0);
        const uint8_t tx = g.in(0x14);
        CHECK(!g.m.bus.lastContended(), "two boards on port 14 are not a contention");
        CHECK(tx == 0x02, "TxRDY of user 1 only (bit 1); hi's nibble is all clear");
        g.out(0x14, 0x52);  // page 14: users 1, 4 and 6
        CHECK(!g.m.bus.lastContended(), "a mask write is not a contention");
        bool okLo = false, okHi = false;
        for (auto& p : g.lo->properties())
            if (p.name == "tx_mask") okLo = p.get().i() == 0x2;
        for (auto& p : g.hi->properties())
            if (p.name == "tx_mask") okHi = p.get().i() == 0x5;
        CHECK(okLo && okHi, "52H loads 2 into the 0-3 board and 5 into the 4-7 board");
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi3), "lo user1 TxINT on VI3");
    }

    SECTION("Interfacer 4 -- a single board: the other nibble floats to 1s (page 15)");
    {
        const char* one = R"(
[machine]
name = "if4one"
[[board]]
type = "interfacer4"
id   = "hi"
)";
        Rig g(one);
        if (!g.hi) return;
        g.out(0x17, 0x04);
        CHECK(g.in(0x15) == 0x0F, "no receive pending on a 4-7 board reads 0FH");
        CHECK(g.in(0x13) == 0xFC, "Status-P drives D0-D1 only; nothing taken, nothing in");
    }

    SECTION("Interfacer 4 -- overlapping lines are still contention");
    {
        const char* clash = R"(
[machine]
name = "if4clash"
[[board]]
type  = "interfacer4"
id    = "hi"
users = "0-3"
[[board]]
type  = "interfacer3"
id    = "if3"
users = "0-7"
)";
        Rig g(clash);
        if (!g.hi) return;
        g.out(0x17, 0);
        g.in(0x14);
        CHECK(g.m.bus.lastContended(), "an Interfacer 3 and 4 in one group both drive D0-D3");
    }

    SECTION("Interfacer 4 -- the Centronics port");
    {
        Rig g;
        if (!g.hi) return;
        g.out(0x17, 0x04);  // exact user 4 = hi relative user 0
        CHECK(g.in(0x11) == 0xC5, "a ready printer reads 0C5H, as the Centronics test waits for");
        g.out(0x10, 'P');
        CHECK(g.lpt->out() == "P", "the byte reaches the printer's line");
        CHECK((g.in(0x11) & 0x81) == 0, "busy during the handshake (D0 and D7 low)");
        CHECK(!g.hi->printerInterrupt(), "no acknowledge yet");
        g.runFor(200);
        CHECK(g.hi->printerInterrupt(), "the end of ACKNLG* sets TxI");
        CHECK(g.in(0x11) == 0xC5, "and the printer is ready again");
        CHECK((g.in(0x14) & 0x10) == 0x10, "port 14 bit 4: TxI of the parallel user");
        CHECK(g.m.bus.viLines() == 0, "masked: no VI line");
        g.out(0x14, 0x10);
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi5), "tx mask bit 4: TxINT on VI5 (JS5)");
        g.out(0x10, 'Q');
        CHECK(g.m.bus.viLines() == 0 && !g.hi->printerInterrupt(), "a data write clears TxI");
        g.runFor(200);
        g.out(0x11, 0x0B);  // the test program's init: AFX, INIT, LED
        bool ctl = false;
        for (auto& p : g.hi->properties())
            if (p.name == "control") ctl = p.get().i() == 0x0B;
        CHECK(ctl, "Control-C latches D0-D3");
        g.hi->reset(Reset::Bus);
        g.lo->reset(Reset::Bus);
        g.i3->reset(Reset::Bus);
        for (auto& p : g.hi->properties())
            if (p.name == "control") ctl = p.get().i() == 0;
        CHECK(ctl, "reset clears Control-C (the LED goes off)");
        CHECK(g.hi->printerInterrupt(), "but not TxI: U16b's only clear is a data write");
        CHECK(g.lpt->out() == "PQ", "the printout, in order");
    }

    SECTION("Interfacer 4 -- the sense switch and the Universal Parallel port");
    {
        Rig g;
        if (!g.hi) return;
        std::string err;
        CHECK(setProperty(*g.hi, "sense", "0x0F", err), "sense = 0FH (S1-3..6 ON)");
        g.out(0x17, 0x04);
        CHECK(g.in(0x10) == 0x0F, "relative port 0 reads the sense switch (page 28 example)");
        g.par->feed("x");
        g.runFor(1000);
        CHECK(g.in(0x13) == 0xFD, "Status-P: DAV (D0), D2-D7 float");
        CHECK((g.in(0x15) & 0x10) == 0x10, "port 15 bit 4: RxI of the parallel user");
        g.out(0x15, 0x10);
        CHECK(g.m.bus.viLines() == viBit(IrqJumper::Vi6), "rx mask bit 4: RxINT on VI6 (JS6)");
        CHECK(g.in(0x12) == 'x', "port 12 reads the input latch");
        CHECK(g.m.bus.viLines() == 0 && (g.in(0x13) & 1) == 0, "and the read clears DAV");
        g.out(0x12, 'y');
        CHECK(g.par->out() == "y", "a write to port 12 goes out on the parallel port");
    }

    SECTION("Interfacer 4 -- the serial channels");
    {
        Rig g;
        if (!g.hi) return;
        g.init(1);
        CHECK((g.in(0x11) & 0xC1) == 0xC1, "TxRDY, and unconnected DSR/DCD read asserted");
        g.out(0x10, 'H');
        CHECK(g.u1->out() == "H", "user 1 is the 0-3 board's U36");
        g.u1->feed("k");
        g.runFor(100000);
        g.out(0x17, 1);
        CHECK(g.in(0x15) == 0x02, "port 15: RxRDY of user 1; the 4-7 board's nibble is clear");
        CHECK(g.in(0x10) == 'k', "and the data port reads it");
    }

    SECTION("Interfacer 4 -- J26 swaps the registers of users 0 and 2, not their status bits");
    {
        Rig g;
        if (!g.hi) return;
        std::string err;
        CHECK(setProperty(*g.hi, "j26", "ac-bd", err), "j26 = ac-bd");
        g.out(0x17, 0x06);  // exact user 6 = relative user 2
        CHECK(g.in(0x11) == 0xC5, "user 6 is now the Centronics port (page 11)");
        g.out(0x10, 'Z');
        g.runFor(200);
        CHECK(g.lpt->out() == "Z", "and prints");
        CHECK((g.in(0x14) & 0x10) == 0x10, "but its TxI is still bit 4 (relative user 0's bit)");
    }

    SECTION("Interfacer 4 -- snapshot, CONFIG SAVE, port");
    {
        Rig g;
        if (!g.hi) return;
        g.out(0x17, 0x06);
        g.out(0x11, 0x08);
        StateWriter w;
        g.hi->serialize(w);
        Rig h;
        if (!h.hi) return;
        StateReader r(w.data());
        h.hi->deserialize(r);
        CHECK(h.hi->selectLatch() == 0x06 && h.hi->selected(), "a snapshot keeps the latch");
        const std::string out = saveTomlText(g.m);
        CHECK(out.find("users = \"0-3\"") != std::string::npos &&
                  out.find("tx_int = \"vi5\"") != std::string::npos,
              "CONFIG SAVE writes the board and unit settings back");
        std::string err;
        CHECK(!setProperty(*g.hi, "port", "0x14", err), "port 14H is refused");
    }

    SECTION("Interfacer 4 -- window titles: the absolute port, or the unit's title");
    {
        Rig g;
        if (!g.hi || !g.lo || !g.i3) return;
        CHECK(g.hi->unitTitle("user3") == "CompuPro Interfacer 4 port 7 (hi)",
              "the default names the absolute port, as MP/M's TTYS does");
        CHECK(g.lo->unitTitle("user1") == "CompuPro Interfacer 4 port 1 (lo)", "users 0-3: port 1");
        CHECK(g.hi->unitTitle("centronics") == "CompuPro Interfacer 4 port 4 centronics (hi)",
              "a parallel unit adds its own name to relative user 0's port");
        CHECK(g.i3->unitTitle("user5") == "CompuPro Interfacer 3 port 13 (if3)",
              "the Interfacer 3 at users 8-15: user5 is port 13");

        std::string err;
        CHECK(setProperty(*g.hi, "j26", "ac-bd", err), "J26 to A-C/B-D");
        CHECK(g.hi->unitTitle("user2") == "CompuPro Interfacer 4 port 4 (hi)" &&
                  g.hi->unitTitle("parallel") == "CompuPro Interfacer 4 port 6 parallel (hi)",
              "J26 swaps the ports of users 0 and 2, and the titles follow");

        bool hasTitle = false;
        for (const auto& p : g.hi->unitProperties("user3")) hasTitle |= p.name == "title";
        CHECK(hasTitle, "every serial unit has a title property");

        std::string seen;
        CHECK(g.hi->connectStream("user3", std::make_unique<TitleSpy>(&seen), err),
              "a line on user3");
        CHECK(setUnitProperty(*g.hi, "user3", "title", "tty1", err), "SET hi:user3 title=tty1");
        CHECK(g.hi->unitTitle("user3") == "tty1", "a set title replaces the default");
        g.m.pump();
        CHECK(seen == "tty1", "the machine hands the line its unit's title");
        CHECK(setUnitProperty(*g.hi, "user3", "title", "", err) && (g.m.pump(), true) &&
                  seen == "CompuPro Interfacer 4 port 7 (hi)",
              "an empty title goes back to the default");

        CHECK(setUnitProperty(*g.hi, "user3", "title", "tty1", err), "title again");
        const std::string out = saveTomlText(g.m);
        CHECK(out.find("title = \"tty1\"") != std::string::npos, "CONFIG SAVE writes the title");
        Machine m2;
        const bool loaded = loadTomlText(out, "if4save", m2, err);
        CHECK(loaded, "the saved file loads");
        auto* hi2 = dynamic_cast<InterfacerFourBoard*>(m2.find("hi"));
        CHECK(hi2 && hi2->unitTitle("user3") == "tty1", "the title comes back from the file");
    }
}
