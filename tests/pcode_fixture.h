#pragma once
// Test support for the WD III.0 P-machine: a small P-code encoder built on the one
// operator table (cpu/pcodeOps.h). There was never a P-code assembler, by design: the
// system and its programs were written in Pascal, and the compiler made the P-code
// (C-1: "Neither Western Digital nor UCSD provide P-code assembler software"). So the
// tests write their programs as text and this turns it into bytes.
//
//   pasm("SLDC 3; SLDC 4; ADI")      -> 03 04 A2
//   pasm("LDL 200; FJP -6")          -> 87 80 C8 D4 FA
//
// Operators are separated by ';' or a newline; parameters by spaces or commas. A number
// is decimal, or hex with 0x. A jump offset is written as the raw offset, relative to the
// next instruction (reference 7.3). A bad line is a test bug, so it throws.

#include "cpu/cpuPmachine.h"
#include "cpu/pcodeOps.h"

#include <array>
#include <cstdio>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pfix {

inline std::vector<uint8_t> pasm(const std::string& src) {
    std::vector<uint8_t> out;
    std::string stmt;
    std::istringstream all(src);
    auto flush = [&](std::string s) {
        for (char& c : s)
            if (c == ',') c = ' ';
        std::istringstream in(s);
        std::string mnem;
        if (!(in >> mnem)) return;
        std::vector<int32_t> args;
        std::string tok;
        while (in >> tok) args.push_back((int32_t)std::stol(tok, nullptr, 0));
        std::string err;
        std::vector<uint8_t> b = altair::pcode::encode(mnem, args, err);
        if (b.empty()) {
            // Printed first: an exception that ends the test run may not show its text.
            std::fprintf(stderr, "pasm: %s in \"%s\"\n", err.c_str(), s.c_str());
            throw std::runtime_error("pasm: " + err + " in \"" + s + "\"");
        }
        out.insert(out.end(), b.begin(), b.end());
    };
    char c;
    while (all.get(c)) {
        if (c == ';' || c == '\n') { flush(stmt); stmt.clear(); }
        else stmt += c;
    }
    flush(stmt);
    return out;
}

// ---------------------------------------------------------------------------
// A small P-machine system in plain memory, for the operator tests.
//
// Memory map (word addresses):
//   FC68        the boot word: points at the boot block      (Pascal-100 manual F 3.7.2)
//   00F0..00F2  boot block: CTP, SDP, RQP
//   0100        the TIB                                      (C-34, III.0 layout)
//   0200        SSV, the system segment vector (128 SIB pointers)
//   0280        the task's SIB vector for segments 128..255   (TIB word 11)
//   0300 + 8n   SIB n
//   0800        segment 2: the execution-error handler (procedures 1 and 2 are NOPs)
//   1000        segment 1: the program under test (procedure 1, more if given)
//   2000        splow: the stack's lower limit
//   5000        the running frame (mp); msstat -> 5400 -> 5800 (bp, the globals)
// The stack starts empty at sp = 5000 and grows toward 2000.
//
// A segment is laid out as real code files are (reference 7.2): word 0 = offset to the
// dictionary word; the constant block from word 1; each procedure as a header word (its
// data size) and its code; then the procedure entries and the dictionary word.

struct PMem : altair::WordPort {
    std::vector<uint16_t> m = std::vector<uint16_t>(65536, 0);
    bool irq = false;
    uint16_t vec = 0;
    uint16_t readWord(uint16_t a) override { return m[a]; }
    void writeWord(uint16_t a, uint16_t v) override { m[a] = v; }
    bool interruptPending() override { return irq; }
    uint16_t interruptVector() override { irq = false; return vec; }  // the acknowledge
};

// How often each opcode ran, across every test that uses Sys, and how many operators
// had no M0015 time (a missing row in pmachineTiming.cpp). The coverage gate reads them.
inline std::array<long, 256> g_opsRun{};
inline long g_zeroTime = 0;

struct Proc {
    uint16_t dataSz = 0;
    std::string code;  // pasm text
};

struct Seg {
    uint16_t base = 0;
    std::vector<uint16_t> codeStart;  // byte offset (IPC) of each procedure's first op, 1-based
    std::vector<uint16_t> codeEnd;    // byte offset just after each procedure's code
};

struct Sys {
    static constexpr uint16_t TIB = 0x0100, SSV = 0x0200, SIBS = 0x0280, SIB0 = 0x0300;
    static constexpr uint16_t SEG2 = 0x0800, SEG1 = 0x1000, SPLOW = 0x2000;
    static constexpr uint16_t FRAME = 0x5000, MID = 0x5400, GLOBALS = 0x5800;

    PMem mem;
    altair::PMachineCore cpu;
    Seg seg1, seg2;

    uint16_t& w(uint16_t a) { return mem.m[a]; }

    Sys() {
        cpu.attach(&mem);
        // SIBs: segbase at word 0. SSV[n] -> SIB n.
        for (uint16_t n = 0; n < 32; ++n) w(SSV + n) = (uint16_t)(SIB0 + 8 * n);
        w(TIB + 2) = SPLOW;    // splow
        w(TIB + 3) = 0x7000;   // spupr
        w(TIB + 11) = SIBS;    // sibs, for segments 128..255
        seg2 = load(2, SEG2, {}, {{0, "NOP"}, {0, "NOP"}});
        // Frames: running -> middle -> global.
        w(FRAME + 0) = MID;      w(FRAME + 1) = GLOBALS;
        w(MID + 0) = GLOBALS;    w(MID + 1) = GLOBALS;
        w(GLOBALS + 0) = GLOBALS; w(GLOBALS + 1) = GLOBALS;
    }

    // Place a code segment at `base` and point SIB `segno` at it.
    Seg load(uint16_t segno, uint16_t base, const std::vector<uint16_t>& consts,
             const std::vector<Proc>& procs) {
        Seg s;
        s.base = base;
        s.codeStart.push_back(0);
        s.codeEnd.push_back(0);
        uint16_t cur = 1;
        for (uint16_t c : consts) w(base + cur++) = c;
        std::vector<uint16_t> entries;
        for (const Proc& p : procs) {
            entries.push_back(cur);
            w(base + cur++) = p.dataSz;
            std::vector<uint8_t> code = pasm(p.code);
            uint32_t byteAt = 2u * cur;
            s.codeStart.push_back((uint16_t)byteAt);
            for (uint8_t c : code) {
                uint16_t& word = w((uint16_t)(base + byteAt / 2));
                word = (byteAt & 1) ? (uint16_t)((word & 0x00FF) | (c << 8))
                                    : (uint16_t)((word & 0xFF00) | c);
                ++byteAt;
            }
            s.codeEnd.push_back((uint16_t)byteAt);
            cur = (uint16_t)((byteAt + 1) / 2);
        }
        uint16_t n = (uint16_t)procs.size();
        uint16_t i0 = (uint16_t)(cur + n);
        for (uint16_t k = 1; k <= n; ++k) w((uint16_t)(base + i0 - k)) = entries[k - 1];
        w((uint16_t)(base + i0)) = (uint16_t)((n << 8) | segno);
        w(base) = i0;
        uint16_t sib = segno < 128 ? w((uint16_t)(SSV + segno)) : w((uint16_t)(SIBS + segno - 128));
        if (sib == 0) {  // segments 128..255 get a SIB on demand
            sib = (uint16_t)(SIB0 + 8 * (segno % 32 + 32));
            w((uint16_t)(SIBS + segno - 128)) = sib;
        }
        w(sib) = base;
        return s;
    }

    // Load `code` as procedure 1 of segment 1 and boot into it through FC68.
    void prog(const std::string& code, std::vector<uint16_t> consts = {},
              std::vector<Proc> more = {}) {
        std::vector<Proc> procs{{8, code}};
        procs.insert(procs.end(), more.begin(), more.end());
        seg1 = load(1, SEG1, consts, procs);
        boot(SEG1, seg1.codeStart[1]);
    }

    void boot(uint16_t segb, uint16_t ipc) {
        w(altair::PMachineCore::kInitVector) = 0x00F0;
        w(0x00F0) = TIB;
        w(0x00F1) = SSV;
        w(0x00F2) = altair::PMachineCore::kNil;
        w(TIB + 4) = FRAME;  // sp: empty stack
        w(TIB + 5) = FRAME;  // mp
        w(TIB + 6) = GLOBALS;
        w(TIB + 7) = ipc;
        w(TIB + 8) = segb;
        cpu.reset(altair::Reset::PowerOn);
        cpu.step();  // the boot sequence
    }

    // One instruction, counted for the coverage gate.
    altair::StepResult step() {
        bool ran = !cpu.halted();
        if (ran) {
            uint32_t at = ((uint32_t)cpu.segb_ * 2 + cpu.ipc_) & 0x1FFFF;
            uint16_t word = w((uint16_t)(at >> 1));
            ++g_opsRun[(at & 1) ? (word >> 8) : (word & 0xFF)];
        }
        altair::StepResult r = cpu.step();
        if (ran && !cpu.stopped() && cpu.lastTime10() == 0) ++g_zeroTime;  // no M0015 row
        cycles += r.tStates;
        return r;
    }
    long cycles = 0;  // 2.5 MHz cycles of the steps after the boot

    // Run until procedure 1 of segment 1 reaches its end, or the core halts. Returns true
    // if it reached the end.
    bool run(int maxSteps = 10000) {
        for (int i = 0; i < maxSteps; ++i) {
            if (cpu.segb_ == SEG1 && cpu.ipc_ == seg1.codeEnd[1]) return true;
            if (cpu.halted()) return false;
            step();
        }
        return false;
    }

    uint16_t tos(int k = 0) { return w((uint16_t)(cpu.sp_ + k)); }
    int depth() { return FRAME - cpu.sp_; }
    uint16_t& local(int i) { return w((uint16_t)(FRAME + 3 + i)); }
    uint16_t& global(int i) { return w((uint16_t)(GLOBALS + 3 + i)); }
    uint16_t& mid(int i) { return w((uint16_t)(MID + 3 + i)); }
};

} // namespace pfix
