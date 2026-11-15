#include "test.h"

#include "boards/s100-memory.h"
#include "cli/monitor.h"
#include "config/toml.h"
#include "core/debug.h"
#include "core/machine.h"

#include <functional>
#include <sstream>
#include <vector>

using namespace altair;

namespace {

// ---------------------------------------------------------------------------
// A DMA controller, in miniature -- the bus-mastering analogue of test_debug's
// IntBoard, and the proof that DESIGN.md 4.5 is real.
//
// It DECODES NOTHING: like a CPU card, a bus master ORIGINATES cycles, it does not
// answer them (test_cpu asserts the CPU card decodes no address and no port; the
// same is true here). It pulls pHOLD when it has a block to move, and once the run
// loop grants pHLDA it drives real RAM->RAM cycles through the very
// BusMaster::step(Bus&) the CPU uses -- so a DMA transfer is just cycles on the
// backplane, snooped and observed like any other.
//
// HAS-A, NOT IS-A (board.h): the board CONTAINS a BusMaster (mover_) and returns it
// from busMaster(); it does NOT inherit BusMaster the way the CPU card does. If it
// did, Machine::master()/masters() -- a dynamic_cast to BusMaster, also how the
// monitor lists the machine's CPUs -- would mistake this controller for a processor
// and try to step it as one.
//
// The one knob that matters is the GRAIN, and it lives entirely in requestsBus():
//   burst       -- hold pHOLD true for the whole block; serviceDma's while-loop
//                  drains it in a single grant.
//   cycle-steal -- drop pHOLD after one byte and re-arm a Clock deadline, so the CPU
//                  runs instructions between DMA cycles.
// The run loop knows about neither mode. That is the whole point.
// ---------------------------------------------------------------------------
class DmaBoard : public Board {
public:
    DmaBoard() { mover_.d = this; }

    // Program a transfer and assert pHOLD. gap is the cycle-steal re-arm interval in
    // T-states (ignored for a burst).
    void start(uint16_t src, uint16_t dst, uint16_t len, bool cycleSteal, uint64_t gap = 40) {
        src_ = src;
        dst_ = dst;
        remaining_ = len;
        cycleSteal_ = cycleSteal;
        gap_ = gap;
        hold_ = len > 0;
        holdChanged();  // announce pHOLD, or the run loop's cached wire never sees it
    }

    bool     done()  const { return remaining_ == 0; }
    uint64_t moved() const { return moved_; }

    // Per-byte cost. NOT a real chip's timing -- this is a harness board, not a
    // modeled card -- just a concrete nonzero number, so the stolen cycles show up in
    // clock.now() and the run loop's zero-T-state wedge guard is never provoked.
    static constexpr uint32_t kPerByte = 8;  // ~ a memory read cycle + a write cycle

    std::string type() const override { return "test-dma"; }
    std::vector<Property> properties() override { return {}; }

    bool       requestsBus() const override { return hold_; }
    BusMaster* busMaster()         override { return &mover_; }
    int        tmaPriority() const override { return prio_; }

    // IEEE 696 arbitration (2.8.3): the TMA switches (-1 = none, a pre-696 card), a
    // shared log of which board moved each byte, and a hook run on the first byte.
    int                       prio_ = -1;
    std::vector<std::string>* log   = nullptr;
    std::function<void()>     onFirst;

private:
    // One granted byte: two REAL bus cycles, so the memory board actually decodes
    // them and any observer/breakpoint watching the stream sees them.
    StepResult transferOne(Bus& bus) {
        uint8_t v = bus.memRead(src_++);
        bus.memWrite(dst_++, v);
        if (log) log->push_back(id);
        if (moved_ == 0 && onFirst) onFirst();
        ++moved_;
        if (--remaining_ == 0) {
            hold_ = false;  // block done: release pHOLD for good
            holdChanged();
        } else if (cycleSteal_) {
            hold_ = false;  // yield the bus now...
            holdChanged();
            clock_->after(gap_, [this] {  // ...and ask for it again later
                hold_ = true;
                holdChanged();
            });
        }
        // burst: hold_ stays true, so serviceDma takes the next byte in the same grant
        return {kPerByte, RunStatus::Ok};
    }

    struct Mover : public BusMaster {
        DmaBoard* d = nullptr;
        StepResult step(Bus& bus) override { return d->transferOne(bus); }
    } mover_;

    uint16_t src_ = 0, dst_ = 0;
    uint32_t remaining_ = 0, moved_ = 0;
    bool     hold_ = false, cycleSteal_ = false;
    uint64_t gap_ = 40;
};

// 64K of RAM and an 8080, built by hand -- same shape as test_debug's Rig, with a
// DMA controller attached and a recognizable pattern laid down for it to move.
struct Rig {
    Machine  m;
    DmaBoard dma;

    static constexpr uint16_t kSrc = 0x2000, kDst = 0x3000, kLen = 16;

    Rig() {
        std::string err;
        m.bus.setVerify(true);  // re-derive decode + the interrupt wire every cycle

        Board* b = m.add("memory", "mem0", err);
        auto* mem = dynamic_cast<MemoryBoard*>(b);
        Region r;
        r.kind = RegionKind::Ram;
        r.at = 0;
        r.size = 0x10000;
        mem->addRegion(r, err);
        setProperty(*mem, "fill", "zero", err);
        mem->power();

        m.add("z80", "cpu0", err);
        m.cpu()->reset(Reset::PowerOn);

        // The controller goes into a slot like any card. It needs the clock for its
        // cycle-steal re-arm; a burst never looks at it.
        dma.id = "dma0";
        dma.attachClock(&m.clock);
        m.bus.attach(&dma);

        // A distinctive source block, and a destination that is still zero.
        for (uint16_t i = 0; i < kLen; ++i) m.bus.memWrite(kSrc + i, (uint8_t)(0x40 + i));
    }

    void load(std::initializer_list<uint8_t> code, uint16_t at = 0) {
        for (uint8_t byte : code) m.bus.memWrite(at++, byte);
    }
    bool destMatchesSource(uint16_t len = kLen) {
        for (uint16_t i = 0; i < len; ++i)
            if (m.bus.memRead(kDst + i) != (uint8_t)(0x40 + i)) return false;
        return true;
    }
};

} // namespace

void test_dma() {
    SECTION("DMA -- a board becomes a bus master, and the clock notices (DESIGN.md 4.5)");

    // A BURST transfer, triggered before the run. The CPU program is NOP ; HLT, so we
    // can read the arithmetic exactly: the NOP retires, the run loop reaches an
    // instruction boundary, and the whole 16-byte block is stolen in a single grant.
    {
        Rig g;
        g.load({0x00, 0x76});  // NOP ; HLT
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, Rig::kLen, /*cycleSteal=*/false);

        RunResult r = g.m.debug.run(0);

        CHECK(r.why == StopReason::Halted, "the CPU still reaches its HLT -- DMA did not derail it");
        CHECK(r.steps == 2, "and retired exactly its two instructions, NOP and HLT");
        CHECK(g.dma.done() && g.dma.moved() == Rig::kLen, "the whole block moved");
        CHECK(g.destMatchesSource(), "byte for byte, RAM->RAM, through real bus cycles");

        // On the Z80 NOP is 4 T-states and HALT is 4. The 16-byte burst steals 16 * 8 = 128 more. The
        // CPU did the SAME two instructions either way -- but 128 extra T-states of
        // emulated time elapsed, which is the CPU genuinely losing the bus.
        CHECK(r.tStates == 4 + 4, "the CPU's own StepResult counts only its instructions: 8");
        CHECK(g.m.clock.now() == 4u + 128u + 4u,
              "but the clock elapsed 136 -- the 128 stolen T-states are charged to it");
    }

    SECTION("DMA -- the same run with no transfer proves the 128 was the theft");

    {
        Rig g;
        g.load({0x00, 0x76});
        g.m.cpu()->setPc(0);
        // dma NOT started: hold_ is false, serviceDma is inert.
        RunResult r = g.m.debug.run(0);
        CHECK(r.why == StopReason::Halted && r.steps == 2, "identical CPU work");
        CHECK(g.m.clock.now() == 4u + 4u, "and only 8 T-states elapse -- no DMA, no theft");
        CHECK(g.dma.moved() == 0, "the controller sat idle, holding nothing");
    }

    SECTION("cycle-steal -- the CPU runs BETWEEN the stolen cycles, not after them");

    // A tight INR A ; JMP 0 loop that never ends on its own, and an 8-byte transfer
    // that trickles across one byte per grant with a 40 T-state gap. Driving the
    // machine one instruction at a time, we count how many instructions the CPU
    // retires while the block is still in flight.
    {
        Rig g;
        g.load({0x3C, 0xC3, 0x00, 0x00});  // INR A ; JMP 0
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, /*len=*/8, /*cycleSteal=*/true, /*gap=*/40);

        int cpuStepsDuringTransfer = 0;
        for (int guard = 0; guard < 10000 && !g.dma.done(); ++guard) {
            g.m.debug.run(1);  // exactly one CPU instruction, plus its serviceDma
            ++cpuStepsDuringTransfer;
        }

        CHECK(g.dma.done() && g.dma.moved() == 8, "all eight bytes eventually crossed");
        CHECK(g.destMatchesSource(8), "and correctly");
        CHECK(cpuStepsDuringTransfer > 8,
              "the CPU retired MANY more instructions than there were DMA cycles -- it interleaved");
    }

    SECTION("burst vs cycle-steal is the BOARD's choice -- one grant moves the block");

    // The exact same slice loop over a BURST controller: a single CPU instruction is
    // enough for the whole block to move, because the burst holds pHOLD across every
    // byte in one grant. The contrast with the cycle-steal count above is the whole
    // distinction, and the run loop has no mode flag for it.
    {
        Rig g;
        g.load({0x3C, 0xC3, 0x00, 0x00});
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, /*len=*/8, /*cycleSteal=*/false);

        int cpuSteps = 0;
        for (int guard = 0; guard < 10000 && !g.dma.done(); ++guard) {
            g.m.debug.run(1);
            ++cpuSteps;
        }
        CHECK(g.dma.done() && g.dma.moved() == 8, "the block moved");
        CHECK(cpuSteps == 1, "in a single grant -- one CPU instruction sufficed for all eight bytes");
    }

    SECTION("BREAK MEM W catches a DMA write, because a DMA cycle is just a cycle");

    // The debugger's breakpoints watch the bus cycle stream from outside the
    // backplane (DESIGN.md 3.0.3), and a DMA write is on that stream like any other.
    // So a BREAK MEM W on the destination fires on the controller's OWN store, with no
    // CPU instruction touching that address at all -- which is exactly the promise
    // BreakKind::MemWrite's comment makes ("ANY master ... CPU, DMA, or the front
    // panel"), now with a DMA master to make good on it.
    {
        Rig g;
        g.load({0x00, 0x00, 0x00, 0x76});  // NOP sled ; HLT -- the CPU never writes kDst
        g.m.cpu()->setPc(0);
        int bp = g.m.debug.add(BreakKind::MemWrite, Rig::kDst, Rig::kDst);
        g.dma.start(Rig::kSrc, Rig::kDst, Rig::kLen, /*cycleSteal=*/false);

        RunResult r = g.m.debug.run(0);
        CHECK(r.why == StopReason::Breakpoint && r.bp == bp,
              "the DMA's write to the destination tripped the breakpoint");
        CHECK(g.m.bus.memRead(Rig::kDst) == 0x40, "and the write itself landed -- not rolled back");
    }

    // -----------------------------------------------------------------------
    // IEEE 696 TMA0*-TMA3* ARBITRATION (2.8.3). The priority is the card's switches,
    // not its slot; the winner keeps the bus until it lets go; two masters the bus
    // cannot arbitrate refuse to run.
    // -----------------------------------------------------------------------
    SECTION("TMA -- the higher priority gets the bus first, whatever its slot");
    {
        Rig g;
        DmaBoard second;
        second.id = "dma1";
        second.attachClock(&g.m.clock);
        g.m.bus.attach(&second);  // the later slot

        std::vector<std::string> log;
        g.dma.prio_  = 3;  // the earlier slot, the lower priority
        second.prio_ = 9;
        g.dma.log = second.log = &log;

        g.load({0x00, 0x76});  // NOP ; HLT
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, 4, /*cycleSteal=*/false);
        second.start(Rig::kSrc, 0x4000, 4, /*cycleSteal=*/false);
        RunResult r = g.m.debug.run(0);

        const std::vector<std::string> want = {"dma1", "dma1", "dma1", "dma1",
                                               "dma0", "dma0", "dma0", "dma0"};
        CHECK(r.why == StopReason::Halted, "the CPU still reaches its HLT");
        CHECK(log == want, "dma1 (priority 9, later slot) moves its whole block before dma0 (3)");
        CHECK(g.dma.done() && second.done(), "and both blocks move at the same boundary");
        g.m.bus.detach(&second);
    }

    SECTION("TMA -- no preemption: a higher card that asks mid-burst waits for the winner");
    {
        Rig g;
        DmaBoard second;
        second.id = "dma1";
        second.attachClock(&g.m.clock);
        g.m.bus.attach(&second);

        std::vector<std::string> log;
        g.dma.prio_  = 3;
        second.prio_ = 9;
        g.dma.log = second.log = &log;
        // dma0 has the bus alone; on its first byte the higher card starts to ask.
        g.dma.onFirst = [&] { second.start(Rig::kSrc, 0x4000, 4, /*cycleSteal=*/false); };

        g.load({0x00, 0x76});
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, 4, /*cycleSteal=*/false);
        g.m.debug.run(0);

        const std::vector<std::string> want = {"dma0", "dma0", "dma0", "dma0",
                                               "dma1", "dma1", "dma1", "dma1"};
        CHECK(log == want, "dma0 keeps the bus to the end of its burst; dma1 (higher) goes next");
        g.m.bus.detach(&second);
    }

    SECTION("TMA -- two masters with one priority: the run refuses to start");
    {
        Rig g;
        DmaBoard second;
        second.id = "dma1";
        second.attachClock(&g.m.clock);
        g.m.bus.attach(&second);
        g.dma.prio_  = 5;
        second.prio_ = 5;

        g.load({0x00, 0x76});
        g.m.cpu()->setPc(0);
        g.dma.start(Rig::kSrc, Rig::kDst, 4, false);
        RunResult r = g.m.debug.run(0);
        CHECK(r.why == StopReason::TmaConflict, "StopReason::TmaConflict");
        CHECK(r.steps == 0 && g.m.clock.now() == 0, "nothing ran: no instruction, no time");
        CHECK(g.dma.moved() == 0, "and no DMA byte moved");
        CHECK(g.m.debug.tmaConflictWhy().find("dma0 and dma1 both have TMA priority 5") !=
                  std::string::npos,
              ("the reason names both boards: " + g.m.debug.tmaConflictWhy()).c_str());

        second.prio_ = 6;  // a switch moved: the next run goes
        r = g.m.debug.run(0);
        CHECK(r.why == StopReason::Halted && g.dma.done(), "with distinct priorities it runs");
        g.m.bus.detach(&second);
    }

    SECTION("TMA -- a card with no TMA lines cannot share the bus");
    {
        Rig g;
        DmaBoard second;
        second.id = "dma1";
        second.attachClock(&g.m.clock);
        g.m.bus.attach(&second);
        second.prio_ = 5;  // dma0 keeps -1: a pre-IEEE 696 card

        g.load({0x00, 0x76});
        g.m.cpu()->setPc(0);
        RunResult r = g.m.debug.run(0);
        CHECK(r.why == StopReason::TmaConflict, "a pre-696 master beside another: refused");
        CHECK(g.m.debug.tmaConflictWhy().find("dma0 (test-dma) has no TMA0*-TMA3* lines") !=
                  std::string::npos,
              ("the reason names it: " + g.m.debug.tmaConflictWhy()).c_str());

        g.m.bus.detach(&second);  // alone again, it is the only master and runs
        r = g.m.debug.run(0);
        CHECK(r.why == StopReason::Halted, "one master with no TMA lines runs, as before");
    }

    SECTION("TMA -- two Disk 1As: the file is refused, then dma_priority fixes it");
    {
        // Two disk boards at CompuPro's default priority 15. The second has its boot
        // ROM off, so the only problem in the file is the arbitration.
        const std::string head =
            "[machine]\nname = \"t\"\n\n"
            "[[board]]\ntype = \"memory\"\nid = \"mem0\"\n"
            "[[board.region]]\ntype = \"ram\"\nat = 0000\nsize = \"64K\"\n\n"
            "[[board]]\ntype = \"z80\"\nid = \"cpu0\"\n\n"
            "[[board]]\ntype = \"disk1a\"\nid = \"fd0\"\nport = C0\n\n"
            "[[board]]\ntype = \"disk1a\"\nid = \"fd1\"\nport = C4\nboot = false\n";
        std::string err;
        Machine bad;
        CHECK(!loadTomlText(head, "two-disks.toml", bad, err), "the TOML loader refuses it");
        CHECK(err.find("fd0 and fd1 both have TMA priority 15") != std::string::npos,
              ("and says why: " + err).c_str());

        Machine m;
        err.clear();
        CHECK(loadTomlText(head + "dma_priority = 14\n", "two-disks.toml", m, err),
              ("with fd1 at 14 it loads: " + err).c_str());

        Monitor mon(m);
        std::ostringstream o;
        mon.exec("SHOW BUS DMA", o);
        const std::string s = o.str();
        const size_t p15 = s.find("TMA 15  fd0"), p14 = s.find("TMA 14  fd1");
        CHECK(p15 != std::string::npos && p14 != std::string::npos && p15 < p14,
              ("SHOW BUS DMA lists both, highest priority first:\n" + s).c_str());

        std::ostringstream o2;
        mon.exec("SET fd1 dma_priority=15", o2);
        mon.exec("STEP", o2);
        CHECK(o2.str().find("the machine did not run: fd0 and fd1 both have TMA priority 15") !=
                  std::string::npos,
              ("a switch set after the load: STEP refuses, with the reason:\n" + o2.str()).c_str());

        std::ostringstream o3;
        mon.exec("SET fd1 dma_priority=16", o3);
        CHECK(o3.str().find("dma_priority must be 0..15") != std::string::npos && mon.failed(),
              ("dma_priority stops at 15:\n" + o3.str()).c_str());
    }
}
