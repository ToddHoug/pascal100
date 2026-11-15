#include "test.h"

#include "boards/s100-memory.h"
#include "cli/monitor.h"
#include "core/debug.h"
#include "core/expr.h"
#include "core/machine.h"
#include "core/statefile.h"
#include "mcp/server.h"
#include "cpu/cpu.h"
#include "cpu/cpu8085.h"
#include "cpu/cpuZ80.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace altair;

// LIVE PROCESSOR SWITCHING (pascal100 Stage 3). A board with two cores, one active, the
// way the Pascal-100 is built (manual ch. II C.3, ch. IV M-O): the active processor is
// held, the other "begins processing exactly where it last left off".
//
// The test board stands in for the Pascal-100 without its Map, its MicroEngine or its
// ports: core 0 is a Z80 -- active on reset, the one that takes interrupts, like the
// Pascal-100's -- and core 1 is an 8085 standing in for the MicroEngine. One port does
// what U208B and U237's auto-transfer bit do:
//
//   OUT FE, v   bit 0 = the processor-select flip-flop (0 = core 0 "z80", 1 = core 1
//               "8085"); bit 1 = auto-transfer: pINT while core 1 runs hands control
//               to core 0 at the next instruction boundary (ch. IV O).
//
// It exists only in the test binary. The real board is Stage 4.

namespace {

class DualCoreTestBoard : public Board, public BusMaster, public CpuCard {
public:
    DualCoreTestBoard() : z80_(std::make_unique<CpuZ80>()), i8085_(std::make_unique<Cpu8085>()) {}

    std::string type() const override { return "test-dualcore"; }

    // ---- BusMaster: the active core drives the bus; the other is held ----
    StepResult step(Bus& bus) override { return activeCore()->step(bus); }

    // ---- CpuCard ----
    CpuCore* activeCore() override {
        return active_ ? static_cast<CpuCore*>(i8085_.get()) : static_cast<CpuCore*>(z80_.get());
    }
    std::vector<CoreUnit> cores() override { return {{"z80", z80_.get()}, {"8085", i8085_.get()}}; }
    size_t coreCount() const override { return 2; }
    void settleSwitch() override {
        // U208B is reset by pINT through the auto-transfer gate. The wire is level, so
        // the check is too: while pINT is up and auto-transfer is on, core 1 cannot run.
        if (active_ == 1 && auto_ && bus() && bus()->intPending()) active_ = 0;
    }

    // ---- Board: the one port ----
    bool decodes(const BusCycle& c) const override {
        return c.type == Cycle::IoWrite && (c.addr & 0xFF) == kPort;
    }
    void write(const BusCycle& c) override {
        active_ = c.data & 1;
        auto_ = (c.data & 2) != 0;
    }

    void reset(Reset r) override {
        z80_->reset(r);
        i8085_->reset(r);
        active_ = 0;   // reset gives control to core 0, as the Pascal-100 gives it to the Z80
        auto_ = false;
    }
    void power() override { reset(Reset::PowerOn); }

    std::vector<Property> properties() override { return {}; }
    std::vector<UnitDef> units() const override {
        return {{"z80", UnitKind::Cpu, active_ == 0 ? "active" : "(idle)"},
                {"8085", UnitKind::Cpu, active_ == 1 ? "active" : "(idle)"}};
    }

    void serialize(StateWriter& w) const override {
        Board::serialize(w);
        z80_->serialize(w);
        i8085_->serialize(w);
        w.u8((uint8_t)active_);
        w.u8(auto_ ? 1 : 0);
    }
    void deserialize(StateReader& r) override {
        Board::deserialize(r);
        z80_->deserialize(r);
        i8085_->deserialize(r);
        active_ = r.u8() & 1;
        auto_ = r.boolean();
    }

    CpuCore* z80() { return z80_.get(); }
    CpuCore* i8085() { return i8085_.get(); }
    int active() const { return active_; }

    static constexpr uint8_t kPort = 0xFE;

private:
    std::unique_ptr<CpuZ80> z80_;
    std::unique_ptr<Cpu8085> i8085_;
    int active_ = 0;
    bool auto_ = false;
};

// A board that pulls pINT (the shape of test_debug.cpp's IntBoard). An OUT to port 10
// raises it too, so a guest can make the interrupt arrive in the MIDDLE of a run -- the
// case only the boundary settle can catch (the run-start settle sees a wire that was
// already up when RUN was typed).
class PintBoard : public Board {
public:
    std::string type() const override { return "test-pint"; }
    bool assertsInt() const override { return raised_; }
    void raise(bool on) {
        raised_ = on;
        intChanged();
    }
    bool decodes(const BusCycle& c) const override {
        return c.type == Cycle::IoWrite && (c.addr & 0xFF) == 0x10;
    }
    void write(const BusCycle&) override { raise(true); }
    std::vector<Property> properties() override { return {}; }

private:
    bool raised_ = false;
};

struct SwitchRig {
    Machine m;
    DualCoreTestBoard* card = nullptr;
    PintBoard* pint = nullptr;

    SwitchRig() {
        std::string err;
        m.bus.setVerify(true);
        Board* b = m.add("memory", "mem0", err);
        auto* mem = dynamic_cast<MemoryBoard*>(b);
        Region r;
        r.kind = RegionKind::Ram;
        r.at = 0;
        r.size = 0x10000;
        mem->addRegion(r, err);
        setProperty(*mem, "fill", "zero", err);
        mem->power();
        card = new DualCoreTestBoard;
        card->id = "cpu0";
        m.adopt(std::unique_ptr<Board>(card));
        pint = new PintBoard;
        pint->id = "int0";
        m.adopt(std::unique_ptr<Board>(pint));
        card->power();
    }
    void load(uint16_t at, std::initializer_list<uint8_t> code) {
        for (uint8_t byte : code) m.bus.memWrite(at++, byte);
    }
    uint8_t peek(uint16_t a) { return m.bus.peek(a); }
};

// THE QB PATTERN (SBQB.ASM, the Pascal-100's own Z80 side): the Z80 hands control to the
// other processor with an OUT and, when control comes back, resumes at the instruction
// after that OUT -- `FROMMICRO: JMP QB`. The other processor leaves a request in a
// mailbox (ZMCOM's Z80FCTN), switches, and finds the answer there when it resumes.
//
//   core 0, Z80, from reset at 0000          core 1, 8085, from 0100
//   0000 LD A,1     ; TOMICRO                 0100 MVI A,2A
//   0002 OUT (FE),A ; -> core 1               0102 STA 8000   ; the request
//   0004 LD A,(8000); FROMMICRO / QB          0105 MVI A,0
//   0007 INC A                                0107 OUT FE     ; -> core 0
//   0008 LD (8001),A; the answer              0109 LDA 8001   ; resumes here
//   000B JP 0000                              010C HLT
void loadMailbox(SwitchRig& g) {
    g.load(0x0000, {0x3E, 0x01, 0xD3, 0xFE, 0x3A, 0x00, 0x80, 0x3C, 0x32, 0x01, 0x80,
                    0xC3, 0x00, 0x00});
    g.load(0x0100, {0x3E, 0x2A, 0x32, 0x00, 0x80, 0x3E, 0x00, 0xD3, 0xFE, 0x3A, 0x01, 0x80,
                    0x76});
    g.card->z80()->setPc(0x0000);
    g.card->i8085()->setPc(0x0100);
}

} // namespace

void test_coreswitch() {
    SECTION("live switching -- the mailbox round trip (the QB pattern)");
    {
        SwitchRig g;
        loadMailbox(g);
        CHECK(g.m.cpu() == g.card->z80(), "reset leaves core 0 (the Z80) active");

        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted, "the 8085 halts at the end with nothing to wake it");
        CHECK(g.m.cpu() == g.card->i8085(), "and the 8085 is the active core when it does");
        CHECK(g.peek(0x8000) == 0x2A && g.peek(0x8001) == 0x2B,
              "the request went one way and the answer came back");
        CHECK(r.pc == 0x010D, "the stop reports the ACTIVE core's PC (past the 8085's HLT)");
        CHECK(g.card->z80()->pc() == 0x0004,
              "the Z80 is held where it left off: after its OUT, at FROMMICRO");
    }

    SECTION("live switching -- history records each instruction on the core that ran it");
    {
        SwitchRig g;
        loadMailbox(g);
        g.m.debug.clearInsnHistory();
        g.m.debug.run(100);
        auto recs = g.m.debug.insnHistory(100);
        const std::vector<uint16_t> want = {0x0000, 0x0002,                          // Z80
                                            0x0100, 0x0102, 0x0105, 0x0107,          // 8085
                                            0x0004, 0x0007, 0x0008, 0x000B, 0x0000,  // Z80
                                            0x0002,                                  //
                                            0x0109, 0x010C};                         // 8085
        bool same = recs.size() == want.size();
        for (size_t i = 0; same && i < want.size(); ++i) same = recs[i].pc == want[i];
        CHECK(same, "the recorded PCs follow the switches, core by core");
    }

    SECTION("live switching -- a PC breakpoint with no core acts on the active core");
    {
        SwitchRig g;
        loadMailbox(g);
        int bp = g.m.debug.add(BreakKind::Pc, 0x0008, 0x0008);   // the Z80's answer store
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp,
              "it stops on the Z80 after the Z80 got control back from the 8085");
        CHECK(r.pc == 0x0008, "at the Z80's PC");
        CHECK(g.m.cpu() == g.card->z80(), "with the Z80 active");
        CHECK(g.card->i8085()->pc() == 0x0109, "and the 8085 held after its OUT");
    }
    {
        // The other way round, which is the case a loop that cached its core at run start
        // gets wrong: the run STARTS on the Z80 and the breakpoint is on the 8085.
        SwitchRig g;
        loadMailbox(g);
        int bp = g.m.debug.add(BreakKind::Pc, 0x0105, 0x0105);   // the 8085, after its request
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp,
              "a run that starts on the Z80 stops at a PC the 8085 reaches");
        CHECK(r.pc == 0x0105 && g.m.cpu() == g.card->i8085(), "on the 8085, at its PC");
        CHECK(g.peek(0x8000) == 0x2A && g.peek(0x8001) == 0x00,
              "after the 8085's request and before the Z80 answered");
    }

    SECTION("live switching -- auto-transfer: pINT hands control back with no OUT");
    {
        // core 0: IM 1, EI, then hand off with auto-transfer on, and spin when resumed.
        //   0000 IM 1 ; 0002 EI ; 0003 LD A,3 ; 0005 OUT (FE),A ; 0007 JR 0007
        //   0038 LD A,55 ; 003A LD (8002),A ; 003D HALT     -- the RST 38 handler
        // core 1: 0100 JMP 0100
        SwitchRig g;
        g.load(0x0000, {0xED, 0x56, 0xFB, 0x3E, 0x03, 0xD3, 0xFE, 0x18, 0xFE});
        g.load(0x0038, {0x3E, 0x55, 0x32, 0x02, 0x80, 0x76});
        g.load(0x0100, {0xC3, 0x00, 0x01});
        g.card->z80()->setPc(0x0000);
        g.card->i8085()->setPc(0x0100);

        int bp = g.m.debug.add(BreakKind::Pc, 0x0100, 0x0100);
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp && g.m.cpu() == g.card->i8085(),
              "the 8085 is running its loop, auto-transfer on");
        g.m.debug.clear();

        g.pint->raise(true);
        r = g.m.debug.run(100);
        CHECK(g.m.cpu() == g.card->z80(), "pINT gave control back to the Z80");
        CHECK(g.peek(0x8002) == 0x55, "which resumed after its OUT and took the interrupt");
        CHECK(r.why == StopReason::Halted, "and halted in its handler");
        CHECK(g.card->i8085()->pc() == 0x0100, "the 8085 is held at its loop");
    }

    SECTION("live switching -- pINT rising DURING a run hands control back at the next boundary");
    {
        // As above, but the 8085 raises the interrupt itself (OUT 10) partway through ONE
        // run. Nothing executed a switch; only the settle at the instruction boundary can
        // see it.
        //   core 1: 0100 OUT 10 ; 0102 JMP 0102
        SwitchRig g;
        g.load(0x0000, {0xED, 0x56, 0xFB, 0x3E, 0x03, 0xD3, 0xFE, 0x18, 0xFE});
        g.load(0x0038, {0x3E, 0x55, 0x32, 0x02, 0x80, 0x76});
        g.load(0x0100, {0xD3, 0x10, 0xC3, 0x02, 0x01});
        g.card->z80()->setPc(0x0000);
        g.card->i8085()->setPc(0x0100);
        g.m.debug.clearInsnHistory();
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted && g.m.cpu() == g.card->z80() && g.peek(0x8002) == 0x55,
              "the Z80 took the interrupt and halted in its handler, all in one run");
        CHECK(g.card->i8085()->pc() == 0x0102, "the 8085 was held right after its OUT 10");
        auto recs = g.m.debug.insnHistory(100);
        bool noSpin = true;
        for (const auto& rec : recs) noSpin = noSpin && rec.pc != 0x0102;
        CHECK(noSpin, "the 8085 never ran its loop: the switch came at the boundary after the OUT");
    }

    SECTION("live switching -- a stop on the switching instruction reports the new core's PC");
    {
        // A CONDITIONAL cycle breakpoint lets the instruction retire and stops at its
        // boundary. On the Z80's OUT (FE),A that means AFTER the switch: the 8085 is active,
        // and the stop must say where IT is, not where the held Z80 is.
        SwitchRig g;
        loadMailbox(g);
        auto regs = g.card->z80()->registers();
        auto known = [regs](const std::string& n) {
            for (const RegDef& rd : regs)
                if (rd.name == n) return true;
            return false;
        };
        std::string err;
        auto c = Expr::parse("A==1", known, err);
        int bp = g.m.debug.add(BreakKind::IoWrite, 0xFE, 0xFE, c);
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "the Z80's OUT (FE),A with A==1 stops");
        CHECK(g.m.cpu() == g.card->i8085() && r.pc == 0x0100,
              "after the switch, at the 8085's PC (0100), not the held Z80's (0004)");
    }

    SECTION("live switching -- each history record names the core that ran it");
    {
        SwitchRig g;
        loadMailbox(g);
        g.m.debug.clearInsnHistory();
        g.m.debug.run(100);
        auto recs = g.m.debug.insnHistory(100);
        bool right = !recs.empty();
        for (const auto& rec : recs)
            right = right && rec.core == (rec.pc >= 0x0100 ? g.card->i8085() : g.card->z80());
        CHECK(right, "a Z80 address carries the Z80, an 8085 address the 8085 (the regions do not overlap)");
    }

    SECTION("live switching -- a cycle breakpoint just after a switch unwinds the right core");
    {
        // BREAK MEM W 8001 is the Z80's LD (8001),A at 0008 -- the first store the Z80 makes
        // after the 8085 handed control back. The veto unwinds it: the Z80's registers go
        // back to the boundary, and the 8085's are not touched.
        SwitchRig g;
        loadMailbox(g);
        int bp = g.m.debug.add(BreakKind::MemWrite, 0x8001, 0x8001);
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "it stops before the store");
        CHECK(r.pc == 0x0008 && g.m.cpu() == g.card->z80(), "with the Z80 on the store");
        CHECK(g.peek(0x8001) == 0x00, "nothing was written");
        CHECK(g.card->i8085()->pc() == 0x0109, "the 8085 is still held after its OUT");
        g.m.debug.clear();
        r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted && g.peek(0x8001) == 0x2B,
              "and a plain resume runs the store and the rest of the round trip");
    }

    SECTION("live switching -- the cycle one-shot belongs to the core that tripped it");
    {
        // Both cores run the same bytes: 0200 OUT FE (D3 FE is OUT in both instruction
        // sets). BREAK IO W FE stops the Z80 there; the operator then selects the 8085,
        // which sits at the SAME address. Its OUT is a new hit, not the one-shot's.
        SwitchRig g;
        g.load(0x0200, {0xD3, 0xFE, 0x76});
        g.card->z80()->setPc(0x0200);
        g.card->i8085()->setPc(0x0200);
        int bp = g.m.debug.add(BreakKind::IoWrite, 0xFE, 0xFE);
        RunResult r = g.m.debug.run(10);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp && g.m.cpu() == g.card->z80(),
              "the Z80's OUT FE trips it");
        g.m.bus.ioWrite(0xFE, 0x01);   // the operator selects core 1 between runs
        CHECK(g.m.cpu() == g.card->i8085(), "the 8085 is active, at the same 0200");
        r = g.m.debug.run(10);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp && g.m.cpu() == g.card->i8085() &&
                  r.pc == 0x0200,
              "the 8085's own OUT FE stops it too: the Z80's one-shot did not let it through");
    }

    SECTION("live switching -- NEXT's target belongs to the core that set it");
    {
        // A step-over target set while the Z80 is active is the Z80's return address. The
        // 8085 reaching that address is not the CALL returning.
        SwitchRig g;
        loadMailbox(g);
        g.m.debug.setStepTarget(0x0105);   // an address only the 8085 reaches
        RunResult r = g.m.debug.run(100);
        g.m.debug.setStepTarget(-1);
        CHECK(r.why == StopReason::Halted, "the 8085 at 0105 does not end the Z80's NEXT");
    }

    SECTION("live switching -- removing the CPU board forgets the history of its cores");
    {
        SwitchRig g;
        loadMailbox(g);
        g.m.debug.run(100);
        CHECK(!g.m.debug.insnHistory(10).empty(), "the run recorded history");
        std::string err;
        g.m.remove("cpu0", err);
        CHECK(g.m.debug.insnHistory(10).empty(),
              "and BOARDS REMOVE took it away with the cores it points at");
    }

    SECTION("live switching -- SNAPSHOT/RESTORE keeps both cores and the select latch");
    {
        SwitchRig g;
        loadMailbox(g);
        g.m.debug.add(BreakKind::Pc, 0x0105, 0x0105);   // the 8085 active, the Z80 held at 0004
        g.m.debug.run(100);
        g.m.debug.clear();
        std::string path = (std::filesystem::temp_directory_path() / "coreswitch.snp").string();
        std::string err;
        CHECK(g.m.snapshot(path, err), "snapshot taken with the 8085 active");

        g.m.debug.run(100);   // run the round trip to its end
        std::vector<std::string> notes;
        CHECK(g.m.restore(path, err, notes), "restored");
        CHECK(g.m.cpu() == g.card->i8085() && g.card->i8085()->pc() == 0x0105 &&
                  g.card->z80()->pc() == 0x0004,
              "the 8085 active at 0105 again, the Z80 held at 0004");
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted && g.peek(0x8001) == 0x2B,
              "and the round trip runs again from there");
        std::filesystem::remove(path);
    }

    // Both cores run the SAME bytes at 0200 (00 = NOP, D3 FE = OUT FE, 76 = HLT in both
    // instruction sets), so one address is reached by each core in turn:
    //   Z80:  0300 LD A,1 ; 0302 JP 0200 ; 0200 NOP ; 0201 OUT (FE),A -> the 8085
    //   8085: 0200 NOP ; 0201 OUT FE (A=0) -> the Z80, which resumes at 0203 HLT
    auto loadShared = [](SwitchRig& g) {
        g.load(0x0300, {0x3E, 0x01, 0xC3, 0x00, 0x02});
        g.load(0x0200, {0x00, 0xD3, 0xFE, 0x76});
        g.card->z80()->setPc(0x0300);
        g.card->i8085()->setPc(0x0200);
    };

    SECTION("BREAK <board>:<unit> <addr> -- a PC breakpoint on ONE core");
    {
        SwitchRig g;
        loadShared(g);
        int any = g.m.debug.add(BreakKind::Pc, 0x0200, 0x0200);
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == any && g.m.cpu() == g.card->z80(),
              "unbound: the first core to reach 0200 (the Z80) stops it");
        g.m.debug.clear();
    }
    {
        SwitchRig g;
        loadShared(g);
        std::string canon;
        CHECK(g.m.coreByRef("CPU0:8085", &canon) == g.card->i8085() && canon == "cpu0:8085",
              "a core is named <board>:<unit>, in any case, and listed as the machine spells it");
        CHECK(!g.m.coreByRef("cpu0:me") && !g.m.coreByRef("mem0:8085") && !g.m.coreByRef("8085"),
              "no such unit, not a CPU board, or no board -- no core");
        int bp = g.m.debug.add(BreakKind::Pc, 0x0200, 0x0200);
        CHECK(g.m.debug.setCore(bp, canon), "bound to the 8085");
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp && g.m.cpu() == g.card->i8085() &&
                  r.pc == 0x0200,
              "the Z80 passes 0200 without stopping; the 8085 arriving there stops it");
        CHECK(g.card->z80()->pc() == 0x0203, "with the Z80 held after its OUT");
        CHECK(g.m.debug.breakpoints()[0].describe() == "pc     cpu0:8085 0200",
              "and the listing says which core");
    }
    {
        // A bound breakpoint whose core is not in the machine never fires.
        SwitchRig g;
        loadShared(g);
        int bp = g.m.debug.add(BreakKind::Pc, 0x0200, 0x0200);
        g.m.debug.setCore(bp, "cpu9:8085");
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted, "a breakpoint on a core that is not there never fires");
    }

    SECTION("BREAK, REGS and HISTORY CPU at the monitor, on a two-core board");
    {
        SwitchRig g;
        loadShared(g);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("BREAK cpu0:8085 200", o);
        CHECK(o.str().find("breakpoint 1: pc     cpu0:8085 0200") != std::string::npos,
              "BREAK cpu0:8085 200 sets a core-bound breakpoint");
        o.str("");
        mon.exec("BREAK cpu0:me 200", o);
        CHECK(o.str().find("no core 'cpu0:me'") != std::string::npos && mon.failed(),
              "an unknown core is refused, by name");
        o.str("");
        mon.exec("BREAK cpu0:8085 MEM W 100", o);
        CHECK(o.str().find("a core applies to a PC breakpoint") != std::string::npos,
              "a core on a MEM breakpoint is refused: a bus cycle belongs to the bus");
        o.str("");
        mon.exec("BREAK cpu0:z80 303 IF A==1", o);
        CHECK(o.str().find("breakpoint 2: pc     cpu0:z80 0303 if A==1") != std::string::npos,
              "IF names the bound core's registers");

        g.m.debug.run(100);
        o.str("");
        mon.exec("REGS", o);
        CHECK(o.str().rfind("[8085] ", 0) == 0, "REGS names the active core on a two-core board");
        o.str("");
        mon.exec("BREAK cpu0:z80 203 IF IX==0", o);   // IX is a Z80 register; the 8085 is active
        CHECK(o.str().find("pc     cpu0:z80 0203 if IX==0") != std::string::npos,
              "IF parses against the BOUND core's registers, not the active core's");
        g.m.debug.clear();
        g.m.debug.run(100);   // on to the Z80's HLT, so the history holds both cores
        o.str("");
        mon.exec("HISTORY CPU 10", o);
        std::string h = o.str();
        CHECK(h.find("[z80] ") != std::string::npos && h.find("[8085] ") != std::string::npos,
              "HISTORY CPU names the core of each line");
    }
    {
        // ...and a one-core machine prints exactly what it always did.
        Machine m;
        std::string err;
        Monitor mon(m);
        std::ostringstream o;
        mon.exec("BOARDS ADD 8085 cpu0", o);
        o.str("");
        mon.exec("REGS", o);
        CHECK(o.str().find('[') == std::string::npos, "REGS on a one-core board has no core name");
        CHECK(m.coreUnit(m.cpu()).empty(), "coreUnit is empty on a one-core board");
    }

    SECTION("review: a stop BEFORE the boundary still applies a switch the instruction caused");
    {
        // The 8085 (auto-transfer on) raises pINT with OUT 10, and that OUT also trips a
        // CONDITIONAL cycle breakpoint, which leaves the loop before the boundary settle.
        // The stop must still hand control to the Z80 and report the Z80's PC.
        SwitchRig g;
        g.load(0x0000, {0xED, 0x56, 0xFB, 0x3E, 0x03, 0xD3, 0xFE, 0x18, 0xFE});
        g.load(0x0038, {0x3E, 0x55, 0x32, 0x02, 0x80, 0x76});
        g.load(0x0100, {0x3E, 0x07, 0xD3, 0x10, 0xC3, 0x04, 0x01});   // MVI A,7 ; OUT 10 ; JMP $
        g.card->z80()->setPc(0x0000);
        g.card->i8085()->setPc(0x0100);
        auto regs = g.card->i8085()->registers();
        auto known = [regs](const std::string& n) {
            for (const RegDef& rd : regs)
                if (rd.name == n) return true;
            return false;
        };
        std::string err;
        int bp = g.m.debug.add(BreakKind::IoWrite, 0x10, 0x10, Expr::parse("A==7", known, err));
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp, "the 8085's OUT 10 with A==7 trips it");
        CHECK(g.m.cpu() == g.card->z80() && r.pc == 0x0007,
              "and the stop leaves the Z80 active, at 0007 -- not the held 8085");
    }

    SECTION("review: a condition naming a register the core lacks does not hold");
    {
        // BREAK 0200 IF IX==0, unbound, typed on the Z80 (IX exists). The Z80 reaches 0200
        // with IX=1 (false); the 8085 reaches it too, and an 8085 has no IX. Read as 0, IX==0
        // would stop it there for no reason.
        SwitchRig g;
        loadShared(g);
        for (RegDef& rd : g.card->z80()->registers())
            if (rd.name == "IX") rd.set(1);
        auto regs = g.card->z80()->registers();
        auto known = [regs](const std::string& n) {
            for (const RegDef& rd : regs)
                if (rd.name == n) return true;
            return false;
        };
        std::string err;
        g.m.debug.add(BreakKind::Pc, 0x0200, 0x0200, Expr::parse("IX==0", known, err));
        RunResult r = g.m.debug.run(100);
        CHECK(r.why == StopReason::Halted, "neither core stops: the Z80's IX is 1, the 8085 has no IX");
    }

    SECTION("review: <board>:<unit> names the core of a ONE-core board too");
    {
        Machine m;
        Monitor mon(m);
        std::ostringstream o;
        mon.exec("BOARDS ADD 8085 cpu0", o);
        CHECK(m.coreByRef("cpu0:8085") == m.cpu(), "cpu0:8085 is the one core, as SHOW cpu0 names it");
        o.str("");
        mon.exec("BREAK cpu0:8085 100", o);
        CHECK(o.str().find("breakpoint 1: pc     cpu0:8085 0100") != std::string::npos,
              "and BREAK accepts it");
    }

    SECTION("review 2: a switch pending BETWEEN runs is applied when anyone asks for the core");
    {
        // The auto-transfer setup: the 8085 runs its loop with auto-transfer on. pINT then
        // comes up while the machine is STOPPED. Nothing has run, but U208B has already
        // reset: REGS must show the Z80, the core that runs next.
        SwitchRig g;
        g.load(0x0000, {0xED, 0x56, 0xFB, 0x3E, 0x03, 0xD3, 0xFE, 0x18, 0xFE});
        g.load(0x0100, {0xC3, 0x00, 0x01});
        g.card->z80()->setPc(0x0000);
        g.card->i8085()->setPc(0x0100);
        g.m.debug.add(BreakKind::Pc, 0x0100, 0x0100);
        g.m.debug.run(100);
        g.m.debug.clear();
        g.pint->raise(true);
        CHECK(g.m.cpu() == g.card->z80(), "the pending switch is applied when the core is asked for");
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("REGS", o);
        CHECK(o.str().rfind("[z80] ", 0) == 0, "REGS names the Z80 before anything runs");
    }

    SECTION("review 2: the run tool's stop_at stops on whichever core gets there");
    {
        // Z80: 0300 LD A,1 ; 0302 OUT (FE),A -> the 8085 ; 0304 HALT
        // 8085: 0400 NOP ; 0401 MVI A,0 ; 0403 OUT FE -> the Z80
        // Only the 8085 reaches 0401. NEXT's target (set on the Z80) must not stop there;
        // stop_at's (any core) must.
        auto prog = [](SwitchRig& g) {
            g.load(0x0300, {0x3E, 0x01, 0xD3, 0xFE, 0x76});
            g.load(0x0400, {0x00, 0x3E, 0x00, 0xD3, 0xFE});
            g.card->z80()->setPc(0x0300);
            g.card->i8085()->setPc(0x0400);
        };
        SwitchRig g;
        prog(g);
        g.m.debug.setStepTarget(0x0401, /*anyCore=*/true);
        RunResult r = g.m.debug.run(100);
        g.m.debug.setStepTarget(-1);
        CHECK(r.why == StopReason::StepTarget && g.m.cpu() == g.card->i8085() && r.pc == 0x0401,
              "anyCore: the 8085 reaching 0401 stops the run");
        SwitchRig h;
        prog(h);
        h.m.debug.setStepTarget(0x0401);
        r = h.m.debug.run(100);
        h.m.debug.setStepTarget(-1);
        CHECK(r.why == StopReason::Halted, "NEXT's target, set on the Z80, ignores the 8085 there");
    }

    SECTION("review 2: RUN ends on the other core -- the machine reports THAT core");
    {
        // RUN starts on the Z80 and ends on the 8085's HLT. (pascal100 2026-11-09: the panel
        // HLTA/bus-float check went with the front panel.)
        SwitchRig g;
        g.load(0x0300, {0x3E, 0x01, 0xD3, 0xFE, 0x76});
        g.load(0x0400, {0x76});
        g.card->z80()->setPc(0x0300);
        g.card->i8085()->setPc(0x0400);
        Monitor mon(g.m);
        std::ostringstream o;
        mon.exec("RUN", o);
        CHECK(g.m.cpu() == g.card->i8085() && g.card->i8085()->halted(), "the 8085 halted");
    }

    SECTION("MCP: step reports the core that runs next; breakpoints take a unit");
    {
        SwitchRig g;
        loadShared(g);
        auto call = [](int id, const std::string& tool, const std::string& args) {
            return R"({"jsonrpc":"2.0","id":)" + std::to_string(id) +
                   R"(,"method":"tools/call","params":{"name":")" + tool +
                   R"(","arguments":)" + args + "}}\n";
        };
        std::string script =
            std::string(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})") + "\n" +
            call(2, "step", R"({"count":4})") +   // Z80: 0300 0302 0200 0201 (OUT -> 8085)
            call(3, "regs", "{}") +
            call(4, "breakpoints", R"({"action":"add","kind":"pc","lo":768,"unit":"CPU0:Z80"})") +
            call(5, "breakpoints", R"({"action":"add","kind":"memwrite","lo":1,"unit":"cpu0:z80"})") +
            call(6, "breakpoints", R"({"action":"add","kind":"pc","lo":1,"unit":"cpu0:me"})");
        std::istringstream in(script);
        std::ostringstream out;
        runMcp(g.m, in, out);
        const std::string r = out.str();
        CHECK(g.m.cpu() == g.card->i8085(), "four steps ran the Z80 into its OUT: the 8085 is active");
        CHECK(r.find("\"core\":\"8085\"") != std::string::npos && r.find("\"pc\":512") != std::string::npos,
              "step answers with the 8085 at 0200 -- not the Z80 it started on");
        size_t cores = 0;
        for (size_t at = 0; (at = r.find("\"core\":\"8085\"", at)) != std::string::npos; ++at) ++cores;
        CHECK(cores == 2, "regs names the core too");
        CHECK(r.find("\"unit\":\"cpu0:z80\"") != std::string::npos,
              "breakpoints add binds a pc breakpoint to a unit, spelled as the machine spells it");
        CHECK(r.find("unit applies to kind pc") != std::string::npos, "a unit on a memwrite is refused");
        CHECK(r.find("no core 'cpu0:me'") != std::string::npos, "an unknown unit is refused");
    }
}
