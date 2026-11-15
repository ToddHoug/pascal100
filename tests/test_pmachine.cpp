// The WD III.0 P-machine: the operator table, the `pcode` disassembler, the test encoder
// and (later tasks) the core itself. Every expected value here comes from the period
// documents (reference/80-0130016-00A0_..._Jul82.md) or from real code on the owner's
// Pascal-100 distribution disk, never from another emulator.

#include "test.h"
#include "pcode_fixture.h"

#include "core/statefile.h"
#include "cpu/cpuPmachine.h"
#include "cpu/pcodeOps.h"
#include "isa/isa.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <memory>

#include <set>
#include <string>
#include <vector>

using namespace altair;
using pcode::Par;

namespace {

// Decode from a byte vector, for the table tests.
pcode::Decoded dec(const std::vector<uint8_t>& b, uint32_t at = 0) {
    return pcode::decode(at, [&](uint32_t a) -> uint8_t { return a < b.size() ? b[a] : 0; });
}

std::string dis(const std::vector<uint8_t>& b, uint16_t at = 0, int base = 16) {
    const Disassembler* d = disassemblerFor("pcode");
    if (!d) return "<no pcode disassembler>";
    return d->at(at, [&](uint16_t a) -> uint8_t { return a < b.size() ? b[a] : 0; }, base).text;
}

void test_optable() {
    SECTION("pmachine -- the operator table is Table C-1");

    const auto& t = pcode::opTable();
    CHECK(t.size() == 100, "100 operators (C-3 has 100; RBP left out, reference 7.11)");

    int assigned = 0;
    std::set<int> unassigned;
    for (int op = 0; op < 256; ++op) {
        if (pcode::opInfo((uint8_t)op)) ++assigned;
        else unassigned.insert(op);
    }
    CHECK(assigned == 168, "168 of the 256 byte values are assigned (reference 6.10)");
    std::set<int> want;
    for (int op = 0x40; op <= 0x77; ++op) want.insert(op);
    for (int op = 0xA8; op <= 0xAF; ++op) want.insert(op);
    for (int op = 0xE8; op <= 0xFF; ++op) want.insert(op);
    CHECK(unassigned == want, "unassigned: exactly 40..77, A8..AF, E8..FF");

    // Each row's range is in order and no two rows share an opcode.
    bool ordered = true;
    for (size_t i = 1; i < t.size(); ++i)
        if (t[i].first <= t[i - 1].last) ordered = false;
    CHECK(ordered, "rows are in opcode order and do not overlap");

    // Spot checks against C-1, one per parameter shape.
    auto row = [](uint8_t op) { return pcode::opInfo(op); };
    CHECK(std::string(row(0x9F)->mnem) == "BNOT" && row(0x9F)->npar == 0,
          "9F is BNOT with no parameter (M0015; reference 7.11)");
    CHECK(std::string(row(0x95)->mnem) == "CXI" && row(0x95)->npar == 3 &&
              row(0x95)->par[0] == Par::UB && row(0x95)->par[1] == Par::DB &&
              row(0x95)->par[2] == Par::UB,
          "CXI 95 takes UB1,DB,UB2");
    CHECK(std::string(row(0x83)->mnem) == "LDC" && row(0x83)->par[0] == Par::B &&
              row(0x83)->par[1] == Par::UB,
          "LDC 83 takes B,UB");
    CHECK(std::string(row(0x81)->mnem) == "LDCI" && row(0x81)->par[0] == Par::W, "LDCI 81 takes W");
    CHECK(std::string(row(0x8A)->mnem) == "UJP" && row(0x8A)->par[0] == Par::SB, "UJP 8A takes SB");
    CHECK(std::string(row(0x89)->mnem) == "LOD" && row(0x89)->par[0] == Par::DB &&
              row(0x89)->par[1] == Par::B,
          "LOD 89 takes DB,B");
    CHECK(row(0x20)->shortValue(0x20) == 1 && row(0x2F)->shortValue(0x2F) == 16,
          "SLDL 20..2F are SLDL1..SLDL16");
    CHECK(row(0x30)->shortValue(0x30) == 1 && row(0x3F)->shortValue(0x3F) == 16,
          "SLDO 30..3F are SLDO1..SLDO16");
    CHECK(row(0x00)->shortValue(0x00) == 0 && row(0x1F)->shortValue(0x1F) == 31,
          "SLDC 00..1F are SLDC0..SLDC31");
    CHECK(row(0x78)->shortValue(0x78) == 0 && row(0x7F)->shortValue(0x7F) == 7,
          "SIND 78..7F are SIND0..SIND7");

    // CPU HISTORY peeks maxInsnLen() bytes per operator, so it must be the table's real
    // longest: opcode + parameters, a B or a W at its 2-byte widest.
    int longest = 0;
    for (const pcode::OpInfo& r : t) {
        int len = 1;
        for (int i = 0; i < r.npar; ++i)
            len += (r.par[i] == pcode::Par::B || r.par[i] == pcode::Par::W) ? 2 : 1;
        if (len > longest) longest = len;
    }
    CHECK(longest == PMachineCore().maxInsnLen(),
          "maxInsnLen() is the longest C-1 operator");
}

void test_encode_decode() {
    SECTION("pmachine -- encode and decode agree for every operator");

    // Edge values for each parameter kind.
    auto edges = [](Par k) -> std::vector<int32_t> {
        switch (k) {
        case Par::UB: return {0, 1, 255};
        case Par::DB: return {0, 127};
        case Par::SB: return {-128, -1, 0, 127};
        case Par::B:  return {0, 127, 128, 255, 256, 32767};
        case Par::W:  return {-32768, -1, 0, 32767};
        }
        return {0};
    };

    bool all = true;
    int cases = 0;
    for (const pcode::OpInfo& r : pcode::opTable()) {
        if (r.isShort()) {
            for (int op = r.first; op <= r.last; ++op) {
                std::string err;
                auto b = pcode::encode(r.mnem, {r.shortValue((uint8_t)op)}, err);
                auto d = dec(b);
                ++cases;
                if (b.size() != 1 || b[0] != op || d.info != &r || d.nargs != 1 ||
                    d.args[0] != r.shortValue((uint8_t)op) || d.len != 1)
                    all = false;
            }
            continue;
        }
        // Walk each parameter through its edges, the others held at their first edge.
        for (int p = 0; p < std::max<int>(1, r.npar); ++p) {
            std::vector<int32_t> e = r.npar ? edges(r.par[p]) : std::vector<int32_t>{0};
            for (int32_t v : e) {
                std::vector<int32_t> args;
                for (int i = 0; i < r.npar; ++i) args.push_back(i == p ? v : edges(r.par[i])[0]);
                std::string err;
                auto b = pcode::encode(r.mnem, args, err);
                auto d = dec(b);
                ++cases;
                bool ok = !b.empty() && d.info == &r && d.len == b.size() && d.nargs == r.npar;
                for (int i = 0; ok && i < r.npar; ++i) ok = d.args[i] == args[i];
                if (!ok) all = false;
            }
        }
    }
    CHECK(all, "every operator round-trips through encode() and decode()");
    CHECK(cases > 300, "the round trip covers several hundred cases");

    SECTION("pmachine -- the B parameter's big form (C-1)");
    std::string err;
    CHECK(pcode::encode("LDL", {127}, err) == (std::vector<uint8_t>{0x87, 0x7F}),
          "B = 127 is one byte");
    CHECK(pcode::encode("LDL", {128}, err) == (std::vector<uint8_t>{0x87, 0x80, 0x80}),
          "B = 128 is two bytes: 80 80");
    CHECK(pcode::encode("LDL", {0x1234}, err) == (std::vector<uint8_t>{0x87, 0x92, 0x34}),
          "B = 1234h: first byte 80h + high byte, then the low byte");
    CHECK(pcode::encode("LDL", {32767}, err) == (std::vector<uint8_t>{0x87, 0xFF, 0xFF}),
          "B = 32767 is FF FF");
    CHECK(pcode::encode("LDCI", {-2}, err) == (std::vector<uint8_t>{0x81, 0xFE, 0xFF}),
          "W is low byte first");
    CHECK(dec({0x87, 0x92, 0x34}).args[0] == 0x1234 && dec({0x87, 0x92, 0x34}).len == 3,
          "decode widens a two-byte B");

    SECTION("pmachine -- encode() refuses what C-1 does not allow");
    CHECK(pcode::encode("FOO", {}, err).empty(), "an unknown operator");
    CHECK(pcode::encode("LDL", {32768}, err).empty(), "B above 32767");
    CHECK(pcode::encode("LOD", {128, 1}, err).empty(), "DB above 127");
    CHECK(pcode::encode("UJP", {128}, err).empty(), "SB above 127");
    CHECK(pcode::encode("SLDL", {0}, err).empty(), "SLDL 0 (SLDL is 1..16)");
    CHECK(pcode::encode("ADI", {1}, err).empty(), "a parameter on an operator that has none");
    CHECK(pcode::encode("sldc", {31}, err) == (std::vector<uint8_t>{0x1F}), "names are case-blind");

    SECTION("pmachine -- the test encoder");
    CHECK(pfix::pasm("SLDC 3; SLDC 4; ADI") == (std::vector<uint8_t>{0x03, 0x04, 0xA2}),
          "pasm: three operators");
    CHECK(pfix::pasm("LDL 200\nFJP -6") ==
              (std::vector<uint8_t>{0x87, 0x80, 0xC8, 0xD4, 0xFA}),
          "pasm: newline separator, big B, negative offset");
    CHECK(pfix::pasm("CXG 2,2") == (std::vector<uint8_t>{0x94, 0x02, 0x02}), "pasm: comma");
    CHECK(pfix::pasm("LDCI 0xFC00") == (std::vector<uint8_t>{0x81, 0x00, 0xFC}), "pasm: hex");
}

void test_pcode_disasm() {
    SECTION("pmachine -- the pcode disassembler");

    CHECK(disassemblerFor("pcode") != nullptr, "disassemblerFor(\"pcode\") finds it");
    CHECK(disassemblerFor("PCODE") != nullptr, "the name is case-blind, like the others");
    bool listed = false;
    for (const std::string& s : instructionSets()) listed = listed || s == "pcode";
    CHECK(listed, "instructionSets() names pcode");
    CHECK(assemblerFor("pcode") == nullptr,
          "no pcode assembler (none existed; DISASM only)");

    // Real code: CALC.CODE (owner's distribution disk), segment 1, procedure 5, from code
    // byte 62. Pascal: x := 1.0; y := 0.0; then `if y >= <const> ... ` (reference 7.2).
    std::vector<uint8_t> calc = {0x84, 0x01, 0x01, 0xCC, 0x8E, 0x02, 0x84, 0x03,
                                 0x00, 0xCC, 0x8E, 0x02, 0x84, 0x03, 0xD0, 0x02,
                                 0x83, 0x1B, 0x02, 0xCF, 0xE5, 0xD4, 0x1D};
    const char* want[] = {"LLA 01",     "SLDC 01", "FLT",    "STM 02",  "LLA 03",
                          "SLDC 00",    "FLT",     "STM 02", "LLA 03",  "LDM 02",
                          "LDC 1B,02",  "GEQREAL", "LNOT",   "FJP 0072"};
    // Disassemble as if the run started at code byte 62, so the FJP target is the real one
    // (byte 114: 83 + 2 + 29, reference 7.3).
    // Sized once and copied in, not insert(): GCC 15 -O2 reports a false -Warray-bounds on
    // vector::insert here (CI, 2026-11-09). Same bytes either way.
    std::vector<uint8_t> img(62 + calc.size(), 0x9C);
    std::copy(calc.begin(), calc.end(), img.begin() + 62);
    const Disassembler* d = disassemblerFor("pcode");
    uint16_t a = 62;
    bool same = d != nullptr;
    std::string got;
    for (const char* w : want) {
        if (!same) break;
        Insn in = d->at(a, [&](uint16_t x) -> uint8_t { return x < img.size() ? img[x] : 0; }, 16);
        got += in.text + "; ";
        if (in.text != w) same = false;
        a += in.len;
    }
    CHECK(same, ("real CALC.CODE bytes disassemble as the compiler meant them: " + got).c_str());
    CHECK(a == 62 + calc.size(), "and the lengths add up to the bytes");

    CHECK(dis({0x40}) == "?\?= 40", "an unassigned opcode prints DDT's ?\?= marker");
    CHECK(disassemblerFor("pcode")->at(0, [](uint16_t) -> uint8_t { return 0xFF; }, 16).len == 1,
          "and is one byte long");
    CHECK(dis({0x81, 0x00, 0xFC}) == "LDCI FC00", "LDCI prints its word");
    CHECK(dis({0x87, 0x92, 0x34}) == "LDL 1234", "a big B prints four digits");
    CHECK(dis({0x8A, 0xFE}, 0) == "UJP 0000", "UJP -2 at 0 jumps back to itself");
    CHECK(dis({0x2F}) == "SLDL 10", "SLDL16 prints its value in the console base");
    CHECK(dis({0x2F}, 0, 8) == "SLDL 020", "and in octal");
}

// ---------------------------------------------------------------------------
// The core: boot, loads and stores, integers, logicals, jumps (Task 3 groups).
// Each test runs a hand-assembled program in pfix::Sys (see pcode_fixture.h).

using pfix::Sys;

void test_core_boot() {
    SECTION("pmachine core -- boot from word FC68 (Pascal-100 manual F 3.7.2)");
    {
        Sys s;
        s.prog("NOP");
        CHECK(s.cpu.ctp_ == Sys::TIB && s.cpu.sdp_ == Sys::SSV &&
                  s.cpu.rqp_ == PMachineCore::kNil,
              "CTP, SDP, RQP come from the block that FC68 points at");
        CHECK(s.cpu.sp_ == Sys::FRAME && s.cpu.mp_ == Sys::FRAME && s.cpu.bp_ == Sys::GLOBALS &&
                  s.cpu.segb_ == Sys::SEG1 && s.cpu.ipc_ == s.seg1.codeStart[1],
              "SP, MP, BP, IPC, SEGB come from TIB words 4..8");
        CHECK(!s.cpu.halted() && !s.cpu.stopped(), "and it runs");
        CHECK(s.run(), "the first instruction is at SEGB*2 + IPC (step 20)");
    }
    {
        Sys s;
        s.prog("NOP");
        s.w(PMachineCore::kInitVector) = 0;
        s.cpu.reset(Reset::PowerOn);
        s.cpu.step();
        CHECK(s.cpu.stopped() && s.cpu.stopReason().find("FC68") != std::string::npos,
              "FC68 = 0 (WD's floppy boot) stops the core and says why");
        CHECK(s.cpu.halted() && !s.cpu.haltCanEnd(true, true), "a stopped core stays stopped");
    }
    {
        PMachineCore c;
        c.reset(Reset::PowerOn);
        CHECK(!c.halted(), "reset reads no memory: the core waits to boot");
        StepResult r = c.step();
        CHECK(c.stopped() && r.status == RunStatus::Halted, "with no memory attached it stops");
    }
}

void test_core_loads() {
    SECTION("pmachine core -- constant, local, global, intermediate loads (2.1-2.5)");
    {
        Sys s;
        s.prog("SLDC 0; SLDC 31; LDCN; LDCB 200; LDCI -2; LCA 3");
        CHECK(s.run() && s.depth() == 6, "SLDC LDCN LDCB LDCI LCA push one word each");
        CHECK(s.tos(0) == Sys::SEG1 + 3, "LCA B pushes segb + B");
        CHECK(s.tos(1) == 0xFFFE, "LDCI pushes its word");
        CHECK(s.tos(2) == 200, "LDCB pushes its byte, high byte zero");
        CHECK(s.tos(3) == 0xFC00, "LDCN pushes NIL = FC00");
        CHECK(s.tos(4) == 31 && s.tos(5) == 0, "SLDC 31 and SLDC 0");
    }
    {
        Sys s;
        s.prog("SLDL 1; SLDL 16; LDL 200; LLA 5; SLDC 9; STL 7");
        s.local(1) = 0x1111; s.local(16) = 0x1616; s.local(200) = 0x2000;
        CHECK(s.run() && s.depth() == 4, "SLDL LDL LLA STL");
        CHECK(s.local(7) == 9, "STL B stores TOS at mp + 3 + B and pops it");
        CHECK(s.tos(0) == Sys::FRAME + 8, "LLA 5 pushes mp + mscw_sz - 1 + 5");
        CHECK(s.tos(1) == 0x2000 && s.tos(2) == 0x1616 && s.tos(3) == 0x1111,
              "LDL 200 (a two-byte B), SLDL 16, SLDL 1");
    }
    {
        Sys s;
        s.prog("SLDO 1; SLDO 16; LDO 130; LAO 2; SLDC 5; SRO 3");
        s.global(1) = 0xA1; s.global(16) = 0xA16; s.global(130) = 0xA130;
        CHECK(s.run() && s.depth() == 4, "SLDO LDO LAO SRO work from bp, not mp");
        CHECK(s.global(3) == 5, "SRO stores at bp + 3 + B");
        CHECK(s.tos(0) == Sys::GLOBALS + 5 && s.tos(1) == 0xA130 && s.tos(2) == 0xA16 &&
                  s.tos(3) == 0xA1,
              "LAO 2, LDO 130, SLDO 16, SLDO 1");
    }
    {
        Sys s;
        s.prog("LOD 1,2; LOD 2,3; LDA 1,4; SLDC 7; STR 2,5; LOD 0,1");
        s.mid(2) = 0x0202; s.global(3) = 0x0303; s.local(1) = 0x0101;
        CHECK(s.run() && s.depth() == 4, "LOD LDA STR follow DB static links from mp");
        CHECK(s.global(5) == 7, "STR 2,5 stores two links up");
        CHECK(s.tos(0) == 0x0101, "LOD 0,1 is the running frame");
        CHECK(s.tos(1) == Sys::MID + 7, "LDA 1,4 is an address one link up");
        CHECK(s.tos(2) == 0x0303 && s.tos(3) == 0x0202, "LOD 2,3 and LOD 1,2");
    }
    {
        Sys s;
        s.prog("LLA 2; SLDC 9; STO");
        CHECK(s.run() && s.depth() == 0 && s.local(2) == 9,
              "STO stores TOS through the pointer under it and pops both");
    }
    {
        Sys s;
        s.prog("LDE 1,1; LAE 1,1; SLDC 4; STE 1,2; LDE 130,1", {0x1234, 0});
        s.load(130, 0x3000, {0xBEEF}, {{0, "NOP"}});
        CHECK(s.run() && s.depth() == 3, "LDE LAE STE");
        CHECK(s.tos(0) == 0xBEEF, "LDE 130,1: a segment above 127 is found through TIB word 11");
        CHECK(s.tos(1) == Sys::SEG1 + 1 && s.tos(2) == 0x1234,
              "LAE and LDE address segbase + B of the named segment");
        CHECK(s.w(Sys::SEG1 + 2) == 4, "STE stores there");
    }
}

void test_core_multi() {
    SECTION("pmachine core -- multiple words and bytes (2.7, 2.8)");
    {
        Sys s;
        s.prog("LDC 1,2", {0x3F80, 0x0000});
        CHECK(s.run() && s.depth() == 2 && s.tos(0) == 0x3F80 && s.tos(1) == 0,
              "LDC: the constant at segb + B goes on top (a real's high word, 7.4)");
    }
    {
        Sys s;
        s.prog("LLA 1; LDM 3");
        s.local(1) = 1; s.local(2) = 2; s.local(3) = 3;
        CHECK(s.run() && s.depth() == 3 && s.tos(0) == 1 && s.tos(1) == 2 && s.tos(2) == 3,
              "LDM: the block at the pointer, its first word on top");
    }
    {
        Sys s;
        s.prog("LLA 4; SLDC 7; SLDC 8; STM 2");
        CHECK(s.run() && s.depth() == 0, "STM pops the block and the pointer under it (7.5)");
        CHECK(s.local(4) == 8 && s.local(5) == 7, "TOS goes to the first word");
    }
    {
        Sys s;
        s.prog("LLA 1; SLDC 0; LDB; LLA 1; SLDC 1; LDB; LLA 1; SLDC 3; LDCB 90; STB");
        s.local(1) = 0x4241; s.local(2) = 0x0011;
        CHECK(s.run() && s.depth() == 2, "LDB and STB take a two-word byte pointer");
        CHECK(s.tos(0) == 0x42 && s.tos(1) == 0x41, "byte 0 is the low half, byte 1 the high");
        CHECK(s.local(2) == 0x5A11, "STB byte 3 of local 1 is the high half of local 2");
    }
}

void test_core_records() {
    SECTION("pmachine core -- records, arrays, packed fields (2.9)");
    {
        Sys s;
        s.prog("LLA 4; LLA 1; MOV 3");
        s.local(1) = 5; s.local(2) = 6; s.local(3) = 7;
        CHECK(s.run() && s.depth() == 0 && s.local(4) == 5 && s.local(5) == 6 && s.local(6) == 7,
              "MOV B copies B words from TOS to the pointer under it");
    }
    auto withPtr = [](const std::string& code) {
        auto s = std::make_unique<Sys>();
        s->prog(code);
        s->local(1) = Sys::GLOBALS + 4;  // a pointer to global 1
        s->global(1) = 11; s->global(3) = 0x1234; s->global(10) = 0x10;
        return s;
    };
    {
        auto s = withPtr("SLDL 1; SIND 2; SLDL 1; IND 9; SLDL 1; INC 5; SLDL 1; SIND 0");
        CHECK(s->run() && s->depth() == 4, "SIND IND INC");
        CHECK(s->tos(0) == 11, "SIND 0: the word the pointer points at");
        CHECK(s->tos(1) == Sys::GLOBALS + 9, "INC 5 adds 5 to the pointer");
        CHECK(s->tos(2) == 0x10 && s->tos(3) == 0x1234, "IND 9 and SIND 2 index and load");
    }
    {
        auto s = withPtr("SLDL 1; SLDC 3; IXA 2");
        CHECK(s->run() && s->depth() == 1 && s->tos(0) == Sys::GLOBALS + 4 + 6,
              "IXA B: pointer + index * B");
    }
    {
        auto s = withPtr("SLDL 1; SLDC 9; IXP 4,4");
        CHECK(s->run() && s->depth() == 3, "IXP leaves a three-word packed field pointer");
        CHECK(s->tos(0) == 4 && s->tos(1) == 4 && s->tos(2) == Sys::GLOBALS + 6,
              "element 9 of 4 per word: start bit 4, width 4, word + 2");
    }
    {
        auto s = withPtr("SLDL 1; SLDC 9; IXP 4,4; LDP");
        CHECK(s->run() && s->depth() == 1 && s->tos(0) == 3,
              "LDP fetches the field through the pointer (2.9 note): 1234h bits 4..7 = 3");
    }
    {
        auto s = withPtr("SLDL 1; SLDC 9; IXP 4,4; SLDC 10; STP");
        CHECK(s->run() && s->depth() == 0 && s->global(3) == 0x12A4,
              "STP stores into the field and leaves the other bits");
    }
}

void test_core_logic_int() {
    SECTION("pmachine core -- logicals and integers (2.10, 2.11)");
    auto one = [](const std::string& code, uint16_t want, const char* what) {
        Sys s;
        s.prog(code);
        bool ok = s.run() && s.depth() == 1 && s.tos(0) == want;
        CHECK(ok, what);
    };
    one("LDCI 0x0F0F; LDCI 0x00FF; LAND", 0x000F, "LAND: the whole word");
    one("LDCI 0x0F0F; LDCI 0x00FF; LOR", 0x0FFF, "LOR: the whole word");
    one("LDCI 0x0F0F; LNOT", 0xF0F0, "LNOT: one's complement");
    one("SLDC 0; BNOT", 1, "BNOT false -> true");
    one("SLDC 1; BNOT", 0, "BNOT true -> false");
    one("LDCI 0xFFFE; BNOT", 1, "BNOT keeps bit 0 only (9F, 7.11)");
    one("LDCI -1; SLDC 1; LEUSW", 0, "LEUSW is unsigned: FFFF <= 1 is false");
    one("LDCI -1; SLDC 1; GEUSW", 1, "GEUSW is unsigned: FFFF >= 1");
    one("LDCI -1; SLDC 1; LEQI", 1, "LEQI is signed: -1 <= 1");
    one("SLDC 1; LDCI -1; GEQI", 1, "GEQI: 1 >= -1");
    one("SLDC 4; SLDC 4; EQUI", 1, "EQUI");
    one("SLDC 4; SLDC 5; NEQI", 1, "NEQI");
    one("LDCI -5; ABI", 5, "ABI");
    one("LDCI -32768; ABI", 0x8000, "ABI of -32768 wraps (no overflow error is described)");
    one("SLDC 5; NGI", 0xFFFB, "NGI: two's complement");
    one("LDCI 32767; SLDC 1; ADI", 0x8000, "ADI wraps at 16 bits (2.11 note)");
    one("SLDC 3; SLDC 5; SBI", 0xFFFE, "SBI: TOS-1 - TOS");
    one("LDCI 300; LDCI 300; MPI", 0x5F90, "MPI keeps the low 16 bits");
    one("LDCI -3; SLDC 4; MPI", 0xFFF4, "MPI is signed");
    one("SLDC 7; SLDC 2; DVI", 3, "DVI: TOS-1 div TOS");
    one("LDCI -7; SLDC 2; DVI", 0xFFFD, "DVI truncates toward zero, as Pascal div does");
    one("LDCI -32768; LDCI -1; DVI", 0x8000, "DVI -32768 div -1 wraps");
    one("SLDC 7; SLDC 3; MODI", 1, "MODI");
    one("LDCI -7; SLDC 3; MODI", 2, "MODI of a negative: the Pascal result");
    one("LDCI -6; SLDC 3; MODI", 3, "MODI: -x mod x returns x, not 0 (2.11 comment)");
    one("SLDC 6; SLDC 3; MODI", 0, "MODI of a positive multiple is 0");
    one("SLDC 5; SLDC 1; SLDC 10; CHK", 5, "CHK in range: the bounds go, the value stays");
    {
        Sys s;
        s.prog("SLDC 9; DUP1");
        CHECK(s.run() && s.depth() == 2 && s.tos(0) == 9 && s.tos(1) == 9, "DUP1");
    }
}

// After a Raise, the error handler (segment 2, procedure 2) is entered with the error
// number as its parameter, as CXG 2,2 enters it.
bool raised(Sys& s, uint16_t err, uint16_t retIpc) {
    uint16_t mp = s.cpu.mp_;
    return s.cpu.segb_ == Sys::SEG2 && s.cpu.ipc_ == s.seg2.codeStart[2] && s.w(mp + 4) == err &&
           s.w(mp + 2) == retIpc && (s.w(mp + 3) & 0xFF) == 1 && s.w(mp + 0) == Sys::GLOBALS &&
           s.w(mp + 1) == Sys::FRAME;
}

void test_core_raise() {
    SECTION("pmachine core -- Raise: run-time errors call segment 2, procedure 2 (2.6, 7.9)");
    auto runN = [](Sys& s, int n) { for (int i = 0; i < n; ++i) s.step(); };
    {
        Sys s;
        s.prog("SLDC 7; SLDC 0; DVI");
        runN(s, 3);
        CHECK(raised(s, 6, s.seg1.codeEnd[1]),
              "DVI by 0 raises 6: handler entered, error on its stack, return IPC saved");
        CHECK(s.w(Sys::SIB0 + 8 * 2 + 2) == 1,
              "the call is counted in segment 2's SIB (segrefs)");
        CHECK(s.w(s.cpu.mp_ + 5) == 0 && s.w(s.cpu.mp_ + 6) == 7,
              "the operands stay on the caller's stack, under the error number");
    }
    {
        Sys s;
        s.prog("SLDC 7; SLDC 0; MODI");
        runN(s, 3);
        // Error 1, as for -1 below: the real machine prints Value range error (7.12).
        CHECK(raised(s, PMachineCore::kRangeError, s.seg1.codeEnd[1]), "MODI by 0 raises 1");
    }
    {
        Sys s;
        s.prog("SLDC 7; LDCI -1; MODI");
        runN(s, 3);
        CHECK(raised(s, PMachineCore::kRangeError, s.seg1.codeEnd[1]),
              "MODI by -1 raises 1 too (the real machine prints Value range error)");
    }
    {
        Sys s;
        s.prog("SLDC 20; SLDC 1; SLDC 10; CHK");
        runN(s, 4);
        CHECK(raised(s, 1, s.seg1.codeEnd[1]), "CHK out of range raises 1 (value range error)");
    }
    {
        Sys s;
        s.prog("SLDC 7; SLDC 0; DVI");
        s.w(Sys::TIB + 2) = Sys::FRAME - 3;  // splow: no room for the handler's frame
        runN(s, 3);
        CHECK(s.cpu.stopped() && s.cpu.stopReason().find("while raising") != std::string::npos,
              "a Raise with no stack left for the handler stops the core and says why");
    }
}

void test_core_jumps() {
    SECTION("pmachine core -- jumps count from the next instruction (2.17, 7.3)");
    auto stack = [](const std::string& code, std::vector<uint16_t> want, const char* what,
                    std::vector<uint16_t> consts = {}, int16_t l1 = 0) {
        Sys s;
        s.prog(code, consts);
        s.local(1) = (uint16_t)l1;
        bool ok = s.run() && s.depth() == (int)want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) ok = s.tos((int)i) == want[i];
        CHECK(ok, what);
    };
    stack("SLDC 0; FJP 1; SLDC 9; SLDC 2", {2}, "FJP on false jumps");
    stack("SLDC 1; FJP 1; SLDC 9; SLDC 2", {2, 9}, "FJP on true falls through");
    stack("SLDC 2; FJP 1; SLDC 9; SLDC 2", {2}, "FJP tests bit 0 only: 2 is false");
    stack("SLDC 0; FJPL 1; SLDC 9; SLDC 2", {2}, "FJPL on false jumps");
    stack("UJP 1; SLDC 9; SLDC 2", {2}, "UJP");
    stack("UJPL 1; SLDC 9; SLDC 2", {2}, "UJPL");
    stack("SLDC 4; SLDC 5; EFJ 1; SLDC 9", {}, "EFJ jumps when not equal");
    stack("SLDC 4; SLDC 4; EFJ 1; SLDC 9", {9}, "EFJ falls through when equal");
    stack("SLDC 4; SLDC 4; NFJ 1; SLDC 9", {}, "NFJ jumps when equal");
    stack("SLDC 4; SLDC 5; NFJ 1; SLDC 9", {9}, "NFJ falls through when not equal");
    {
        // A loop with a backward jump: count local 1 down from 3, counting turns in local 2.
        Sys s;
        s.prog("SLDC 3; STL 1; SLDL 2; SLDC 1; ADI; STL 2; SLDL 1; SLDC 1; SBI; DUP1; STL 1;"
               " SLDC 0; EFJ -14");
        CHECK(s.run() && s.local(2) == 3 && s.local(1) == 0 && s.depth() == 0,
              "a backward EFJ loops three times");
    }
    // XJP: table at segb + 1: low 0, high 1, offsets from the next instruction.
    const char* xjp = "SLDL 1; XJP 1; SLDC 10; UJP 4; SLDC 20; UJP 1; SLDC 21";
    stack(xjp, {20}, "XJP case 0", {0, 1, 3, 6}, 0);
    stack(xjp, {21}, "XJP case 1", {0, 1, 3, 6}, 1);
    stack(xjp, {10}, "XJP above the range falls through", {0, 1, 3, 6}, 5);
    stack(xjp, {10}, "XJP below the range falls through", {0, 1, 3, 6}, -1);
    stack("SLDC 1; SLDC 2; SWAP; NOP", {1, 2}, "SWAP; NOP does nothing");
}

void test_core_misc() {
    SECTION("pmachine core -- an unassigned opcode stops the core (6.10)");
    {
        Sys s;
        s.prog("NOP");
        uint16_t a = (uint16_t)(Sys::SEG1 + s.seg1.codeStart[1] / 2);
        s.w(a) = (uint16_t)((s.w(a) & 0xFF00) | 0x40);
        StepResult r = s.step();
        CHECK(s.cpu.stopped() && r.status == RunStatus::Halted, "opcode 40 stops the core");
        CHECK(s.cpu.stopReason().find("40") != std::string::npos, "and the reason names it");
        CHECK(!s.cpu.haltCanEnd(true, true), "no interrupt restarts it");
    }

    SECTION("pmachine core -- registers by reflection, and SNAPSHOT");
    {
        PMachineCore c;
        std::vector<RegDef> regs = c.registers();
        std::vector<std::string> names;
        for (const RegDef& r : regs) names.push_back(r.name);
        CHECK((names == std::vector<std::string>{"SP", "MP", "BP", "SEGB", "CTP", "RQP", "SDP",
                                                 "IPC"}),
              "SP MP BP SEGB CTP RQP SDP IPC, the Pascal-100 manual's names");
        uint32_t seed = 0x2468ACE1u;
        bool same = true;
        for (int round = 0; round < 16 && same; ++round) {
            for (const RegDef& d : regs) { seed = seed * 1664525u + 1013904223u; d.set(seed >> 8); }
            std::vector<uint32_t> fast;
            c.captureRegs(fast);
            same = fast.size() == regs.size();
            for (size_t i = 0; same && i < regs.size(); ++i) same = fast[i] == regs[i].get();
        }
        CHECK(same, "captureRegs() matches registers()");
        regs[7].set(0x1234);
        CHECK(c.pc() == 0x1234, "IPC is pc()");
    }
    {
        Sys s;
        s.prog("SLDC 1; SLDC 2; SLDC 3");
        s.step();
        StateWriter w;
        s.cpu.serialize(w);
        PMachineCore c2;
        StateReader r(w.data());
        c2.deserialize(r);
        CHECK(r.ok() && c2.sp_ == s.cpu.sp_ && c2.ipc_ == s.cpu.ipc_ && c2.segb_ == s.cpu.segb_ &&
                  c2.ctp_ == s.cpu.ctp_ && c2.sdp_ == s.cpu.sdp_ && c2.rqp_ == s.cpu.rqp_ &&
                  c2.mp_ == s.cpu.mp_ && c2.bp_ == s.cpu.bp_ && !c2.halted(),
              "SNAPSHOT/RESTORE round-trips every register and the run state");
        c2.attach(&s.mem);
        c2.step();
        CHECK(c2.sp_ == s.cpu.sp_ - 1 && s.tos(-1) == 2, "and the restored core runs on");
    }
    {
        Sys s;
        s.prog("NOP");
        s.w(PMachineCore::kInitVector) = 0;
        s.cpu.reset(Reset::PowerOn);
        s.cpu.step();
        StateWriter w;
        s.cpu.serialize(w);
        PMachineCore c2;
        StateReader r(w.data());
        c2.deserialize(r);
        CHECK(c2.stopped() && c2.stopReason() == s.cpu.stopReason(),
              "a stopped core restores stopped, with its reason");
    }
}

// ---------------------------------------------------------------------------
// Reals, sets, byte-array compares (Task 4 groups).

// A real as the P-machine stores it (7.4): the high word first.
std::vector<uint16_t> realWords(float f) {
    uint32_t bits = std::bit_cast<uint32_t>(f);
    return {(uint16_t)(bits >> 16), (uint16_t)bits};
}
float stackReal(Sys& s, int k = 0) {
    return std::bit_cast<float>(((uint32_t)s.tos(k) << 16) | s.tos(k + 1));
}
std::vector<uint16_t> reals(std::initializer_list<float> fs) {
    std::vector<uint16_t> out;
    for (float f : fs) for (uint16_t w : realWords(f)) out.push_back(w);
    return out;
}

void test_core_reals() {
    SECTION("pmachine core -- reals (2.12, 2.13)");
    CHECK(realWords(1.0f) == (std::vector<uint16_t>{0x3F80, 0x0000}),
          "the test's real layout is the disk's: 1.0 = 3F80 0000 (7.4)");
    auto r1 = [](const std::string& code, std::vector<uint16_t> consts, float want,
                 const char* what) {
        Sys s;
        s.prog(code, consts);
        CHECK(s.run() && s.depth() == 2 && stackReal(s) == want, what);
    };
    auto i1 = [](const std::string& code, std::vector<uint16_t> consts, uint16_t want,
                 const char* what) {
        Sys s;
        s.prog(code, consts);
        CHECK(s.run() && s.depth() == 1 && s.tos() == want, what);
    };
    r1("SLDC 3; FLT", {}, 3.0f, "FLT 3 -> 3.0 (4040 0000), high word on top");
    r1("LDCI -1; FLT", {}, -1.0f, "FLT -1 -> -1.0");
    i1("LDC 1,2; TNC", reals({2.7f}), 2, "TNC 2.7 -> 2");
    i1("LDC 1,2; TNC", reals({-2.7f}), 0xFFFE, "TNC -2.7 -> -2");
    i1("LDC 1,2; RND", reals({2.5f}), 3, "RND 2.5 -> 3 (half away from zero)");
    i1("LDC 1,2; RND", reals({-2.5f}), 0xFFFD, "RND -2.5 -> -3");
    i1("LDC 1,2; RND", reals({2.4f}), 2, "RND 2.4 -> 2");
    r1("LDC 1,2; ABR", reals({-1.5f}), 1.5f, "ABR");
    r1("LDC 1,2; NGR", reals({1.5f}), -1.5f, "NGR");
    r1("LDC 1,2; LDC 3,2; ADR", reals({1.5f, 2.25f}), 3.75f, "ADR");
    r1("LDC 1,2; LDC 3,2; SBR", reals({1.0f, 3.0f}), -2.0f, "SBR: TOS-1 - TOS");
    r1("LDC 1,2; LDC 3,2; MPR", reals({1.5f, 4.0f}), 6.0f, "MPR");
    r1("LDC 1,2; LDC 3,2; DVR", reals({1.0f, 4.0f}), 0.25f, "DVR: TOS-1 / TOS");
    r1("LDC 1,2; LDC 3,2; SBR", reals({2.0f, 2.0f}), 0.0f, "an exact zero is not an underflow");
    {
        Sys s;
        s.prog("LDC 1,2; DUP2", reals({6.5f}));
        CHECK(s.run() && s.depth() == 4 && stackReal(s, 0) == 6.5f && stackReal(s, 2) == 6.5f,
              "DUP2 copies two words");
    }
    i1("LDC 1,2; LDC 3,2; EQUREAL", reals({2.0f, 2.0f}), 1, "EQUREAL");
    i1("LDC 1,2; LDC 3,2; LEQREAL", reals({2.0f, 3.0f}), 1, "LEQREAL 2 <= 3");
    i1("LDC 1,2; LDC 3,2; LEQREAL", reals({3.0f, 2.0f}), 0, "LEQREAL 3 <= 2 is false");
    i1("LDC 1,2; LDC 3,2; GEQREAL", reals({3.0f, 3.0f}), 1, "GEQREAL is >= (C-1 settles the title)");

    auto err12 = [](const std::string& code, std::vector<uint16_t> consts, int steps,
                    const char* what) {
        Sys s;
        s.prog(code, consts);
        for (int i = 0; i < steps; ++i) s.step();
        CHECK(s.cpu.segb_ == Sys::SEG2 && s.w(s.cpu.mp_ + 4) == 12, what);
    };
    err12("LDC 1,2; LDC 3,2; DVR", reals({1.0f, 0.0f}), 3, "DVR by 0.0 raises 12");
    err12("LDC 1,2; LDC 3,2; MPR", reals({1e30f, 1e30f}), 3, "MPR overflow raises 12");
    // Underflow is NOT an error on the real machine, whatever the manual says (reference 7.12):
    // the result is +0. A result below the smallest normal number goes to +0 too.
    r1("LDC 1,2; LDC 3,2; MPR", reals({1e-30f, 1e-30f}), 0.0f, "MPR underflow gives 0, no error");
    r1("LDC 1,2; LDC 3,2; MPR", reals({1e-30f, 1e-9f}), 0.0f, "a subnormal product gives 0");
    r1("LDC 1,2; LDC 3,2; DVR", reals({1e-30f, 1e30f}), 0.0f, "DVR underflow gives 0");
    r1("LDC 1,2; LDC 3,2; SBR", reals({1.5e-38f, 1.4e-38f}), 0.0f, "a subnormal difference gives 0");
    r1("LDC 1,2; LDC 3,2; MPR", reals({1.4e-38f, 0.9f}), 1.4e-38f * 0.9f, "a small normal result stays");
    {
        Sys s;
        s.prog("LDC 1,2; LDC 3,2; MPR", reals({-1e-30f, 1e-30f}));
        CHECK(s.run() && s.tos(0) == 0 && s.tos(1) == 0, "a negative underflow gives +0: both words 0");
    }
    err12("LDC 1,2; LDC 3,2; ADR", reals({3e38f, 3e38f}), 3, "ADR overflow raises 12");
    err12("LDC 1,2; TNC", reals({40000.0f}), 2, "TNC outside -32768..32767 raises 12 (C-1)");
}

void test_core_sets() {
    SECTION("pmachine core -- sets (2.14, 2.15)");
    auto st = [](const std::string& code, std::vector<uint16_t> want, const char* what) {
        Sys s;
        s.prog(code);
        bool ok = s.run() && s.depth() == (int)want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) ok = s.tos((int)i) == want[i];
        CHECK(ok, what);
    };
    // Sets made with SRS. On the stack: the length on top, then the words, word 0 first.
    const std::string s1_3 = "SLDC 1; SLDC 3; SRS; ";
    const std::string s1_5 = "SLDC 1; SLDC 5; SRS; ";
    const std::string s3_9 = "SLDC 3; SLDC 9; SRS; ";
    const std::string s1_20 = "SLDC 1; SLDC 20; SRS; ";
    const std::string s20 = "SLDC 20; SLDC 20; SRS; ";
    const std::string s1 = "SLDC 1; SLDC 1; SRS; ";
    const std::string s2 = "SLDC 2; SLDC 2; SRS; ";
    const std::string empty = "SLDC 5; SLDC 2; SRS; ";
    // {1..3} in two words (a trailing zero word), from INT of {1..20} and {1..3}.
    const std::string s1_3long = s1_20 + s1_3 + "INT; ";

    st(s1_3, {1, 0x000E}, "SRS 1..3: length 1, bits 1..3");
    st(s20, {2, 0x0000, 0x0010}, "SRS 20..20: length 2, bit 4 of word 1");
    st(empty, {0}, "SRS with low > high: the null set, its length word only");
    st("SLDC 2; " + s1_3 + "INN", {1}, "INN 2 in {1..3}");
    st("SLDC 5; " + s1_3 + "INN", {0}, "INN 5 not in {1..3}");
    st("SLDC 31; " + s1_3 + "INN", {0}, "INN beyond the set's words is false");
    st("LDCI -1; " + s1_3 + "INN", {0}, "INN of a negative value is false");
    st(s1_3 + "SLDC 5; SLDC 6; SRS; UNI", {1, 0x006E}, "UNI, same lengths");
    st(s1 + s20 + "UNI", {2, 0x0002, 0x0010}, "UNI, the top set longer: it moves up (2.14)");
    st(s20 + s1 + "UNI", {2, 0x0002, 0x0010}, "UNI, the lower set longer");
    st(s1_5 + s3_9 + "INT", {1, 0x0038}, "INT {1..5} * {3..9} = {3..5}");
    st(s1_5 + empty + "INT", {0}, "INT with an empty top set");
    st(empty + s1_5 + "INT", {0}, "INT with an empty lower set");
    st(s1_3long, {2, 0x000E, 0x0000}, "INT, the lower set longer: its extra words become 0");
    st(s1_3 + s1_20 + "INT", {1, 0x000E}, "INT, the top set longer: the lower set's length");
    st(s1_5 + s3_9 + "DIF", {1, 0x0006}, "DIF {1..5} - {3..9} = {1,2}");
    st(s1_5 + empty + "DIF", {1, 0x003E}, "DIF of an empty set changes nothing");
    st(empty + s1_5 + "DIF", {0}, "DIF from an empty set is empty");
    st(s1_3 + s1_3 + "EQUPWR", {1}, "EQUPWR equal");
    st(s1_3 + s1_5 + "EQUPWR", {0}, "EQUPWR not equal");
    st(s1_3long + s1_3 + "EQUPWR", {1}, "EQUPWR: trailing zero words do not count");
    st("SLDC 0; SLDC 0; SRS; " + s20 + s20 + "DIF; EQUPWR", {0},
       "EQUPWR {0} = a 2-word empty set is false: a common-word mismatch decides");
    st(s1_3 + s1_5 + "LEQPWR", {1}, "LEQPWR {1..3} <= {1..5}");
    st(s1_5 + s1_3 + "LEQPWR", {0}, "LEQPWR {1..5} <= {1..3} is false");
    st(s1 + s2 + "LEQPWR", {0}, "LEQPWR {1} <= {2} is false (the printed form says true, 2.15)");
    st(s20 + s1_3 + "LEQPWR", {0}, "LEQPWR: a non-zero extra word in the left set");
    st(s1_3long + s1_3 + "LEQPWR", {1}, "LEQPWR: zero extra words in the left set");
    st(s1_3 + s1_20 + "LEQPWR", {1}, "LEQPWR: the right set longer");
    st(s1_5 + s1_3 + "GEQPWR", {1}, "GEQPWR {1..5} >= {1..3}");
    st(s1_3 + s1_5 + "GEQPWR", {0}, "GEQPWR {1..3} >= {1..5} is false");
    st(s2 + s1 + "GEQPWR", {0}, "GEQPWR {2} >= {1} is false");
    st(s1_3 + s20 + "GEQPWR", {0}, "GEQPWR: a non-zero extra word in the right set");
    st(s1_3 + "ADJ 3", {0x000E, 0, 0}, "ADJ expands with zero words and drops the length");
    st(s1_20 + "ADJ 1", {0xFFFE}, "ADJ shrinks by chopping the high words");
    st(s1_3 + "ADJ 1", {0x000E}, "ADJ to the same size drops only the length");
    {
        Sys s;
        s.prog("SLDC 0; LDCI 4080; SRS");
        for (int i = 0; i < 3; ++i) s.step();
        CHECK(s.cpu.segb_ == Sys::SEG2 && s.w(s.cpu.mp_ + 4) == 1,
              "SRS above 4079 raises 1 (value range)");
    }
}

void test_core_bytecmp() {
    SECTION("pmachine core -- byte-array compares (2.16): lexicographic, TOS-1 against TOS");
    auto cmp = [](const char* left, const char* right, const char* op, uint16_t want,
                  const char* what) {
        Sys s;
        s.prog(std::string("LLA 1; LLA 3; ") + op + " 3");
        auto put = [&](int at, const char* t) {  // 3 chars in 2 words, byte 0 = low half
            s.local(at) = (uint16_t)(t[0] | (t[1] << 8));
            s.local(at + 1) = (uint16_t)t[2];
        };
        put(1, left);
        put(3, right);
        CHECK(s.run() && s.depth() == 1 && s.tos() == want, what);
    };
    cmp("AZ ", "BA ", "LEQBYT", 1, "'AZ' <= 'BA' (element-wise, as printed, would say false)");
    cmp("AZ ", "BA ", "GEQBYT", 0, "'AZ' >= 'BA' is false");
    cmp("BA ", "AZ ", "LEQBYT", 0, "'BA' <= 'AZ' is false: the left operand is TOS-1");
    cmp("ABC", "ABC", "LEQBYT", 1, "equal arrays: <=");
    cmp("ABC", "ABC", "GEQBYT", 1, "equal arrays: >=");
    cmp("ABC", "ABC", "EQUBYT", 1, "EQUBYT equal");
    cmp("ABC", "ABD", "EQUBYT", 0, "EQUBYT: the last byte differs");
    cmp("AB\xC1", "AB\x41", "GEQBYT", 1, "bytes compare unsigned");
}

// ---------------------------------------------------------------------------
// Calls, returns, static links, concurrency, processor registers (Task 5 groups).

// A callee that returns its own static link (LSL 1) as a one-word function result:
// data size 0, the result at offset 1 (the word the caller reserved).
const pfix::Proc kRetStatic{0, "LSL 1; STL 1; RPU 0"};

void test_core_calls() {
    SECTION("pmachine core -- calls and return (2.18, 2.19, 7.2, 7.6)");
    {
        // A function with one parameter: data size 1 (local 1), the parameter at offset 2,
        // the result at offset 3. RPU 2 leaves the result (7.2: RPU's B = data + params).
        Sys s;
        s.prog("SLDC 0; LDCB 41; CPL 2", {}, {{1, "SLDL 2; SLDC 1; ADI; STL 3; RPU 2"}});
        s.step(); s.step(); s.step();
        uint16_t mp = s.cpu.mp_;
        CHECK(mp == Sys::FRAME - 2 - 4 - 1 && s.cpu.sp_ == mp,
              "CPL builds the mark stack below the data size (header word) and parameters");
        CHECK(s.cpu.ipc_ == s.seg1.codeStart[2], "and enters at the word after the header (7.2)");
        CHECK(s.w(mp + 0) == Sys::FRAME && s.w(mp + 1) == Sys::FRAME &&
                  s.w(mp + 2) == s.seg1.codeEnd[1] && (s.w(mp + 3) & 0xFF) == 0,
              "msstat = the caller's mp, msdynl = the caller's mp, msipc = return, msseg = 0");
        CHECK(s.run() && s.depth() == 1 && s.tos() == 42 && s.cpu.mp_ == Sys::FRAME,
              "RPU returns: the result stays, mp and IPC come back");
    }
    {
        Sys s;
        s.prog("SLDC 0; CPL 2", {}, {kRetStatic});
        CHECK(s.run() && s.depth() == 1 && s.tos() == Sys::FRAME, "CPL: the static link is mp");
    }
    {
        Sys s;
        s.prog("SLDC 0; CPG 2", {}, {kRetStatic});
        CHECK(s.run() && s.depth() == 1 && s.tos() == Sys::GLOBALS, "CPG: the static link is bp");
    }
    {
        // Main calls 2 (CPL: 2's static link is main's frame). 2 calls 3 with CPI 1,3, as a
        // sibling: 3's static link must be 2's static link, main's frame (7.6).
        Sys s;
        s.prog("SLDC 0; CPL 2", {}, {{0, "SLDC 0; CPI 1,3; STL 1; RPU 0"}, kRetStatic});
        CHECK(s.run() && s.depth() == 1 && s.tos() == Sys::FRAME,
              "CPI DB walks DB links from the CALLER's mp (7.6)");
    }
    auto ext = [](const std::string& code, uint16_t want, const char* what) {
        Sys s;
        s.prog(code);
        s.load(3, 0x3000, {}, {kRetStatic});
        CHECK(s.run() && s.depth() == 1 && s.tos() == want && s.cpu.segb_ == Sys::SEG1, what);
        CHECK(s.w(Sys::SIB0 + 8 * 3 + 2) == 0, "  and segment 3's segrefs is back to 0");
    };
    ext("SLDC 0; CXG 3,1", Sys::GLOBALS, "CXG: another segment, static link bp, back to SEGB 1");
    ext("SLDC 0; CXL 3,1", Sys::FRAME, "CXL: static link mp");
    ext("SLDC 0; CXI 3,1,1", Sys::MID, "CXI UB1,DB,UB2: DB links from the caller's mp");
    ext("SLDC 0; LSL 0; LDCI 0x0301; CPF", Sys::FRAME,
        "CPF: segment in the high byte, procedure in the low, the static link under them");
    {
        Sys s;
        s.prog("SLDC 0; CXG 3,1");
        s.load(3, 0x3000, {}, {kRetStatic});
        s.step(); s.step();
        uint16_t mp = s.cpu.mp_;
        CHECK(s.cpu.segb_ == 0x3000 && (s.w(mp + 3) & 0xFF) == 1 &&
                  s.w(Sys::SIB0 + 8 * 3 + 2) == 1,
              "inside: SEGB is segment 3, msseg = the caller's segment 1, segrefs counts it");
    }
    {
        Sys s;
        s.prog("LSL 0; LSL 1; LSL 2");
        CHECK(s.run() && s.tos(0) == Sys::GLOBALS && s.tos(1) == Sys::MID &&
                  s.tos(2) == Sys::FRAME,
              "LSL DB pushes the frame DB links up (2.20)");
    }
    {
        Sys s;
        s.prog("CPL 2", {}, {{20000, "RPU 0"}});
        s.step();
        CHECK(s.cpu.segb_ == Sys::SEG2 && s.w(s.cpu.mp_ + 4) == 4,
              "a call with no room for its frame raises 4 (stack overflow)");
    }
    // createmscw (2.18): the frame (data_sz + mscw_sz = 100 + 4 words) must fit ONCE between
    // the new sp and splow -- not twice, which the printed test reads as with the new sp
    // (reference 7.12: the real machine lists with the once reading).
    for (int room : {150, 104, 103}) {
        Sys s;
        s.prog("CPL 2", {}, {{100, "RPU 0"}});
        s.w(Sys::TIB + 2) = (uint16_t)(Sys::FRAME - room);  // splow
        s.step();
        bool called = s.cpu.segb_ != Sys::SEG2 && s.cpu.mp_ == Sys::FRAME - 104;
        CHECK(room >= 104 ? called : (s.cpu.segb_ == Sys::SEG2 && s.w(s.cpu.mp_ + 4) == 4),
              (room >= 104 ? "a 104-word frame with " + std::to_string(room) +
                                 " words free is made (it fits once)"
                           : std::string("with 103 words free it raises 4")).c_str());
    }
    {
        Sys s;
        s.prog("BPT");
        s.step();
        CHECK(raised(s, 15, s.seg1.codeEnd[1]), "BPT raises 15 (programmed break-point, 7.9)");
    }
}

void test_core_registers() {
    SECTION("pmachine core -- LPR and SPR (2.22, 7.8)");
    auto one = [](const std::string& code, uint16_t want, const char* what) {
        Sys s;
        s.prog(code);
        CHECK(s.run() && s.tos() == want, what);
    };
    one("LDCI -1; LPR", Sys::TIB, "LPR -1: the current task");
    one("LDCI -2; LPR", Sys::SSV, "LPR -2: the segment vector");
    one("LDCI -3; LPR", PMachineCore::kNil, "LPR -3: the ready queue");
    one("SLDC 11; LPR", Sys::SIBS, "LPR 11: TIB word 11 (sibs)");
    one("SLDC 4; LPR", Sys::FRAME - 1, "LPR 4 saves the live registers first: the live sp");
    one("SLDC 1; LDCB 192; SPR; SLDC 1; LPR", 192, "SPR 1 sets the priority (as SYSTEM.PASCAL)");
    {
        Sys s;
        s.prog("SLDC 4; LDCI 0x4000; SPR");
        CHECK(s.run() && s.cpu.sp_ == 0x4000, "SPR 4 leaves sp at the stored value (7.8 order)");
    }
    {
        Sys s;
        s.prog("SLDC 6; LSL 0; SPR");
        CHECK(s.run() && s.cpu.bp_ == Sys::FRAME, "SPR 6 with LSL 0: bp := mp, as SYSTEM.PASCAL");
    }
    {
        Sys s;
        s.prog("LDCI -2; LDCI 0x0222; SPR");
        CHECK(s.run() && s.cpu.sdp_ == 0x0222 && s.depth() == 0, "SPR -2 sets the segment vector");
    }
    {
        Sys s;
        s.prog("LDCI -4; LPR");
        s.step(); s.step();
        CHECK(s.cpu.stopped(), "LPR of an undefined register (-4) stops the core");
        CHECK(s.cpu.ipc_ == s.seg1.codeStart[1] + 3, "  and IPC stays on the LPR");
    }
    {
        Sys s;
        s.prog("LDCI -5; SLDC 9; SPR");
        s.run();
        CHECK(s.cpu.stopped() && s.cpu.ipc_ == s.seg1.codeStart[1] + 4 && s.depth() == 2,
              "SPR of register -5 stops with IPC on the SPR and the stack unchanged");
    }
}

// A second task's TIB, ready to run procedure 1 of segment 1 from `ipc`.
void makeTib(Sys& s, uint16_t tib, uint8_t prior, uint16_t sp, uint16_t ipc) {
    s.w(tib + 0) = PMachineCore::kNil;
    s.w(tib + 1) = prior;
    s.w(tib + 2) = Sys::SPLOW;
    s.w(tib + 4) = sp;
    s.w(tib + 5) = Sys::FRAME;
    s.w(tib + 6) = Sys::GLOBALS;
    s.w(tib + 7) = ipc;
    s.w(tib + 8) = Sys::SEG1;
}

void test_core_tasks() {
    SECTION("pmachine core -- SIGNAL, WAIT, task switch, idle, interrupt (2.21, 7.7)");
    constexpr uint16_t SEM = 0x0600, TIB2 = 0x0140, TIB3 = 0x0160, TIB4 = 0x0180, VEC = 0x0610;
    {
        Sys s;
        s.prog("LDCI 0x0600; WAIT; SLDC 7");
        s.w(SEM) = 2; s.w(SEM + 1) = PMachineCore::kNil;
        CHECK(s.run() && s.w(SEM) == 1 && s.depth() == 1 && s.tos() == 7,
              "WAIT with a count: decrement it and go on (2.21 note); the address is popped");
    }
    {
        Sys s;
        s.prog("LDCI 0x0600; WAIT; SLDC 7");
        s.w(SEM) = 0; s.w(SEM + 1) = PMachineCore::kNil;
        s.w(Sys::TIB + 1) = 100;
        s.step(); s.step();
        CHECK(s.cpu.idle() && s.cpu.halted(), "WAIT at 0 with no other task: the core idles");
        CHECK(s.w(SEM + 1) == Sys::TIB && s.w(Sys::TIB + 4) == Sys::FRAME &&
                  s.w(Sys::TIB + 7) == s.seg1.codeEnd[1] - 1,
              "the task waits on the semaphore, its TIB holds sp (address popped) and IPC");
        CHECK(!s.cpu.haltCanEnd(false, false) && s.cpu.haltCanEnd(true, false),
              "only an interrupt can end the idle");
        s.mem.irq = true;
        CHECK(s.cpu.haltCanEnd(false, false),
              "the card's interrupt line (the WordPort) can end the idle, not only bus pINT");
        s.mem.irq = false;
        s.step();
        CHECK(s.cpu.idle(), "and with no interrupt it stays idle");
        s.w(VEC) = SEM;
        s.mem.vec = VEC;
        s.mem.irq = true;
        s.step();
        CHECK(!s.cpu.halted() && s.cpu.ctp_ == Sys::TIB && s.w(SEM + 1) == PMachineCore::kNil &&
                  !s.mem.irq,
              "an interrupt signals the vector's semaphore: the waiting task runs again");
        CHECK(s.run() && s.depth() == 1 && s.tos() == 7, "and finishes its program");
    }
    {
        Sys s;
        s.prog("LDCI 0x0600; SIGNAL; SLDC 7");
        s.w(SEM) = 0; s.w(SEM + 1) = PMachineCore::kNil;
        CHECK(s.run() && s.w(SEM) == 1 && s.depth() == 1, "SIGNAL with no waiter counts up");
    }
    {
        Sys s;
        s.prog("LDCI 0x0600; SIGNAL; SLDC 7");
        s.w(Sys::TIB + 1) = 100;
        makeTib(s, TIB2, 10, 0x4800, 0);
        s.w(SEM) = 0; s.w(SEM + 1) = TIB2;
        CHECK(s.run() && s.cpu.ctp_ == Sys::TIB && s.cpu.rqp_ == TIB2 &&
                  s.w(SEM + 1) == PMachineCore::kNil && s.w(SEM) == 0,
              "SIGNAL wakes a lower-priority waiter onto the ready queue; no switch");
    }
    {
        Sys s;
        s.prog("LDCI 0x0600; SIGNAL; SLDC 7");
        s.w(Sys::TIB + 1) = 100;
        makeTib(s, TIB2, 200, 0x4800, s.seg1.codeEnd[1] - 1);
        s.w(SEM) = 0; s.w(SEM + 1) = TIB2;
        s.step(); s.step();
        CHECK(s.cpu.ctp_ == TIB2 && s.cpu.sp_ == 0x4800 && s.cpu.rqp_ == Sys::TIB,
              "SIGNAL wakes a higher-priority waiter: it runs, the signaller goes to the queue");
        CHECK(s.w(Sys::TIB + 4) == Sys::FRAME && s.w(Sys::TIB + 7) == s.seg1.codeEnd[1] - 1,
              "the signaller's TIB holds its sp (address popped) and its next IPC");
    }
    {
        // Ready queue A(50) -> B(10); a woken C(30) goes between them.
        Sys s;
        s.prog("LDCI 0x0600; SIGNAL");
        s.w(Sys::TIB + 1) = 100;
        makeTib(s, TIB3, 50, 0x4800, 0);
        makeTib(s, TIB4, 10, 0x4800, 0);
        makeTib(s, TIB2, 30, 0x4800, 0);
        s.w(TIB3) = TIB4;
        s.cpu.rqp_ = TIB3;
        s.w(SEM) = 0; s.w(SEM + 1) = TIB2;
        CHECK(s.run() && s.cpu.rqp_ == TIB3 && s.w(TIB3) == TIB2 && s.w(TIB2) == TIB4 &&
                  s.w(TIB4) == PMachineCore::kNil,
              "the ready queue stays in priority order: 50, 30, 10");
    }
    {
        Sys s;
        s.prog("LDCI 0x0600; WAIT");
        s.w(SEM) = 0; s.w(SEM + 1) = PMachineCore::kNil;
        constexpr uint16_t SEM2 = 0x0620;
        s.w(SEM2) = 0; s.w(SEM2 + 1) = PMachineCore::kNil;
        s.step(); s.step();
        s.w(VEC) = SEM2;
        s.mem.vec = VEC;
        s.mem.irq = true;
        s.step();
        CHECK(s.cpu.idle() && s.w(SEM2) == 1 && !s.mem.irq,
              "an interrupt for a semaphore nobody waits on counts it, and the core idles again");
    }
    {
        Sys s;
        s.prog("LDCI -1; LDCI 0x0140; SPR");
        makeTib(s, TIB2, 5, 0x4800, s.seg1.codeEnd[1]);
        CHECK(s.run() && s.cpu.ctp_ == TIB2 && s.cpu.sp_ == 0x4800 &&
                  s.w(Sys::TIB + 4) == Sys::FRAME,
              "SPR -1: the old task's registers are saved, the new task runs from its TIB");
    }
}

// ---------------------------------------------------------------------------
// M0015 timing (Task 6). Each expected time is the sum of the M0015 times of the program's
// operators, worked out by hand from the printed rows (us); at 2.5 MHz a cycle is 0.4 us.

void test_core_short_forms() {
    SECTION("pmachine core -- every short form (SLDC 0..31, SLDL 1..16, SLDO 1..16, SIND 0..7)");
    bool ok = true;
    for (int v = 0; v <= 31; ++v) {
        Sys s;
        s.prog("SLDC " + std::to_string(v));
        ok = ok && s.run() && s.depth() == 1 && s.tos() == v;
    }
    CHECK(ok, "SLDC n pushes n");
    ok = true;
    for (int n = 1; n <= 16; ++n) {
        Sys s;
        s.prog("SLDL " + std::to_string(n) + "; SLDO " + std::to_string(n));
        s.local(n) = (uint16_t)(0x100 + n);
        s.global(n) = (uint16_t)(0x200 + n);
        ok = ok && s.run() && s.tos(1) == 0x100 + n && s.tos(0) == 0x200 + n;
    }
    CHECK(ok, "SLDL n and SLDO n load local n and global n");
    ok = true;
    for (int k = 0; k <= 7; ++k) {
        Sys s;
        s.prog("LLA 1; SIND " + std::to_string(k));
        s.local(1 + k) = (uint16_t)(0x300 + k);
        ok = ok && s.run() && s.depth() == 1 && s.tos() == 0x300 + k;
    }
    CHECK(ok, "SIND k loads the word k past the address");
}

void test_core_timing() {
    SECTION("pmachine core -- M0015 timing (hand-computed from the printed rows)");
    // The cycles a whole program takes: floor(total tenths / 4), as the odd tenths carry.
    auto t = [](const std::string& code, std::vector<uint16_t> consts, std::vector<pfix::Proc> more,
                double us, const char* what) {
        Sys s;
        s.prog(code, consts, more);
        s.run(100);
        long want = (long)std::lround(us * 10) / 4;
        std::string msg = std::string(what) + ": " + std::to_string(s.cycles) +
                          " cycles, want " + std::to_string(want);
        CHECK(s.cycles == want, msg.c_str());
    };
    t("SLDC 3; SLDC 4; ADI", {}, {}, 2.8 + 2.8 + 8.0, "fixed rows: SLDC, ADI");
    t("SLDC 0; SLDC 0; IXA 1", {}, {}, 5.6 + 9.6, "IXA, index 0: the printed best case");
    t("SLDC 0; SLDC 1; IXA 1", {}, {}, 5.6 + 18.7, "IXA 1, index 1: measured");
    t("SLDC 0; SLDC 7; IXA 100", {}, {}, 5.6 + 29.1, "IXA 100, index 7: measured");
    t("SLDC 0; SLDC 7; IXA 5", {}, {}, 5.6 + 24.6, "IXA 5: between rows, the lower row");
    t("SLDC 0; SLDC 3; IXA 1", {}, {}, 5.6 + 19.3, "IXA 1, a 2-bit index: half way, an estimate");
    t("SLDC 0; LDCI 32767; IXA 100", {}, {}, 2.8 + 8.4 + 56.8, "IXA: never above the printed 56.8");
    t("LOD 2,1", {}, {}, 17.2 + 3.2 * 2, "LOD: 17.2 + 3.2(DB)");
    t("LDC 1,2", reals({1.0f}), {}, 18.0 + 4.0 * 2, "LDC: 18.0 + 4.0(UB)");
    t("SLDC 3; FLT", {}, {}, 2.8 + (44.8 - 2.0 * 1), "FLT 3: 44.8 - 2.0(trunc(lg 3))");
    t("LDCI -1; FLT", {}, {}, 8.4 + (44.8 - 0 + 2.0), "FLT -1: C = 2.0 for a negative");
    t("SLDC 0; FLT", {}, {}, 2.8 + 10.8, "FLT 0");
    t("LDC 1,2; TNC", reals({2.7f}), {}, 26.0 + (48.0 - 0.8 * 1), "TNC 2.7");
    t("LDC 1,2; RND", reals({-2.5f}), {}, 26.0 + (48.8 - 0.8 * 1 + 0.8), "RND -2.5");
    t("LDC 1,2; TNC", reals({0.75f}), {}, 26.0 + 50.0, "TNC 0.75: 0.5 <= abs < 1.0");
    t("SLDC 7; SLDC 2; DVI", {}, {}, 5.6 + 89.2, "DVI, positive result");
    t("LDCI -7; SLDC 2; DVI", {}, {}, 8.4 + 2.8 + 91.2, "DVI, negative result");
    t("LDCI -7; SLDC 3; MODI", {}, {}, 8.4 + 2.8 + 92.8, "MODI, negative dividend");
    t("SLDC 0; SLDC 5; MPI", {}, {}, 5.6 + 5.2, "MPI n * 0");
    t("SLDC 1; SLDC 3; SRS", {}, {}, 5.6 + (50.4 + 2.0 * 1 + 2.0 * 3), "SRS, 1 word set");
    t("SLDC 20; SLDC 20; SRS", {}, {}, 5.6 + (52.4 + 2.0 * 4 + 2.0 * 4 + 1.6), "SRS, 2 word set");
    t("SLDC 1; SLDC 3; SRS; SLDC 1; SLDC 3; SRS; EQUPWR", {}, {}, 64.0 * 2 + (16.0 + 7.6 * 1),
      "EQUPWR: 16.0 + 7.6(N), N = 1");
    t("SLDC 1; SLDC 3; SRS; SLDC 5; SLDC 2; SRS; UNI", {}, {}, 64.0 + 23.6 + 6.6,
      "UNI with a null TOS: 6.6 is not a whole cycle, so 0.2 us carries");
    t("SLDC 1; SLDC 3; SRS; SLDC 5; SLDC 2; SRS; UNI; SLDC 5; SLDC 2; SRS; UNI", {}, {},
      64.0 + 23.6 + 6.6 + 23.6 + 6.6, "  two of them: the carried tenths make a whole cycle");
    t("SLDC 0; FJP 0", {}, {}, 2.8 + 16.8, "FJP, jump");
    t("SLDC 1; FJP 0", {}, {}, 2.8 + 10.8, "FJP, no jump");
    t("CPL 2", {}, {{0, "RPU 0"}}, 45.6 + 26.0, "CPL + RPU");
    t("CPI 1,2", {}, {{0, "RPU 0"}}, (53.6 + 3.2) + 26.0, "CPI: 53.6 + 3.2(DB)");
    {
        Sys s;
        s.prog("LDCI 0x0600; SIGNAL");
        s.w(0x0601) = PMachineCore::kNil;
        s.run();
        CHECK(s.cycles == (long)std::lround((8.4 + 18.0) * 10) / 4, "SIGNAL, no waiter, count 0");
    }
    t("LDCI -1; LPR", {}, {}, 8.4 + 8.4, "LPR of a negative register");
    {
        Sys s;
        s.prog("LLA 1; LLA 3; LEQBYT 3");
        s.local(1) = 'A' | ('Z' << 8); s.local(3) = 'B' | ('A' << 8);
        s.run();
        CHECK(s.cycles == (long)std::lround((7.6 * 2 + 16.8 + 10.4) * 10) / 4,
              "LEQBYT 'AZ' <= 'BA': LESS, L = 1: 16.8 + 10.4((L+1) div 2)");
    }
    {
        // 6.6 us leaves 2 tenths of carry. A reset must not keep it.
        Sys s;
        s.prog("SLDC 1; SLDC 3; SRS; SLDC 5; SLDC 2; SRS; UNI");
        s.run();
        s.cpu.reset(altair::Reset::PowerOn);
        s.cpu.step();  // boot
        s.cycles = 0;
        s.run();
        CHECK(s.cycles == 942 / 4, "reset clears the carried tenths");
    }
    {
        Sys s;
        s.prog("BPT");
        s.step();
        CHECK(s.cycles == 632 / 4, "BPT: the Raise only, comparable to CXG (63.2)");
    }
    {
        Sys s;
        s.prog("SLDC 1; SLDC 0; DVI");
        s.run(3);
        CHECK(s.cycles == (long)std::lround((5.6 + 8.4 + 63.2) * 10) / 4,
              "DVI by 0: its shortest time plus the Raise");
    }
    {
        Sys s;
        constexpr uint16_t SEM = 0x0600, VEC = 0x0610;
        s.prog("LDCI 0x0600; WAIT; SLDC 7");
        s.w(SEM) = 0; s.w(SEM + 1) = PMachineCore::kNil;
        s.step(); s.step();
        CHECK(s.cycles == (long)std::lround((8.4 + 90.8) * 10) / 4, "WAIT with a task switch");
        long before = s.cycles;
        s.step();
        CHECK(s.cycles == before + 1, "an idle step counts one cycle");
        s.w(VEC) = SEM; s.mem.vec = VEC; s.mem.irq = true;
        before = s.cycles;
        s.step();
        CHECK(s.cycles == before + 1348 / 4, "an interrupt is timed as a SIGNAL with a task switch");
    }
}

void test_core_coverage() {
    SECTION("pmachine core -- coverage gate: every assigned opcode ran, and took time");
    std::string missing;
    int assigned = 0;
    for (int op = 0; op < 256; ++op) {
        if (!pcode::opInfo((uint8_t)op)) continue;
        ++assigned;
        if (pfix::g_opsRun[op] == 0) {
            char b[8];
            std::snprintf(b, sizeof b, " %02X", op);
            missing += b;
        }
    }
    CHECK(assigned == 168, "168 assigned opcodes");
    std::string msg = "every assigned opcode ran in a test; not run:" + missing;
    CHECK(missing.empty(), msg.c_str());
    CHECK(pfix::g_zeroTime == 0, "every operator that ran has an M0015 time");
}

} // namespace

void test_pmachine() {
    test_optable();
    test_encode_decode();
    test_pcode_disasm();
    test_core_boot();
    test_core_loads();
    test_core_multi();
    test_core_records();
    test_core_logic_int();
    test_core_raise();
    test_core_jumps();
    test_core_misc();
    test_core_reals();
    test_core_sets();
    test_core_bytecmp();
    test_core_calls();
    test_core_registers();
    test_core_tasks();
    test_core_short_forms();
    test_core_timing();
    test_core_coverage();  // last: it reads what the tests above ran
}
