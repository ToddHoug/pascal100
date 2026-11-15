// PMachineCore timing: WD Technical Note M0015, the execution time of each operator, in the
// Pascal-100 manual's Appendix H (Reference/pascal100/manual-appH-system-tables.md).
//
// Times are in tenths of a microsecond, as M0015 prints them (7.2 us = 72). step() turns
// them into 2.5 MHz cycles. Where M0015 prints a formula, the formula is used. Where it
// prints only a range with no formula, the core uses, in this order: the printed "typical"
// time, else the middle of the printed range. Each such place is marked "no formula" and
// is listed in docs/devguide/pmachine.md; the real machine can settle them.
//
// time10() runs before the operator, so it reads the operands as M0015 names them: TOS is
// the top of the stack, TOS-1 the word (or real, or set) under it.

#include "cpu/cpuPmachine.h"
#include "cpu/pcodeOps.h"

#include <cmath>
#include <cstdlib>

namespace altair {

namespace {

// IXA with an index that is not 0. M0015 prints only 9.6 to 56.8 and no formula, so these
// times are measured: a Pascal-100 at 3.0 MHz, K:=A[Z] timed from the host, scaled to 2.5 MHz
// (Pascal100-Altairsim/speed-test/RESULTS.md in the project notes). Rows are the element sizes
// measured; a size between two rows takes the lower row. Only indexes 1 and 7 were measured. A
// longer index adds the same time for each bit, up to the printed worst case, and that part is
// an estimate.
uint32_t ixaTime10(int32_t size, int32_t index) {
    static const struct { int32_t size; uint32_t at1, at7; } kRows[] = {
        {1, 187, 199}, {2, 190, 221}, {3, 193, 239}, {4, 191, 246},
        {8, 191, 273}, {16, 209, 291}, {100, 213, 291},
    };
    const auto* row = &kRows[0];
    for (const auto& r : kRows)
        if (std::abs(size) >= r.size) row = &r;
    int bits = 0;
    for (uint32_t n = (uint32_t)std::abs(index); n; n >>= 1) ++bits;
    uint32_t t = row->at1 + (row->at7 - row->at1) * (uint32_t)(bits - 1) / 2;
    return t < 568 ? t : 568;
}

// The middle of a printed range, for the rows with no formula.
constexpr uint32_t mid(uint32_t lo, uint32_t hi) { return (lo + hi) / 2; }

// ((n+1) div 2) * 10.4 + ((n+1) mod 2) * 2.8: the byte-compare loop (2 bytes a pass).
uint32_t bytePasses(int32_t n) { return 104 * ((n + 1) / 2) + 28 * ((n + 1) % 2); }

// trunc(lg(abs(x))) for abs(x) >= 1: the binary exponent.
int lg(float x) { return std::ilogb(x); }

} // namespace

uint32_t PMachineCore::signalTime10(uint16_t sem) {
    uint16_t count = rd(sem), waitq = rd((uint16_t)(sem + 1));
    if (waitq == kNil) return count > 0 ? 148 : 180;
    if (count != 0) return 148;  // a count with waiters does not occur; timed as a count
    // A waiter wakes. A task switch follows if it outranks the running task, or if there
    // is no running task (an interrupt at label 6).
    bool sw = ctp_ == kNil || prior(ctp_) < prior(waitq);
    return sw ? 1348 : 520;
}

uint32_t PMachineCore::time10(const pcode::Decoded& d) {
    const uint8_t op = d.op;
    const int32_t* a = d.args;
    auto S = [this](int k) { return rd((uint16_t)(sp_ + k)); };  // TOS = S(0)
    auto I = [&](int k) { return (int16_t)S(k); };

    if (op <= 0x1F) return 28;                         // SLDC
    if (op <= 0x2F) return 64;                         // SLDL
    if (op <= 0x3F) return 72;                         // SLDO
    if (op >= 0x78 && op <= 0x7F) return 84;           // SIND

    switch (op) {
    // Constant one word loads
    case 0x98: return 64;                              // LDCN
    case 0x80: return 56;                              // LDCB
    case 0x81: return 84;                              // LDCI
    case 0x82: return 80;                              // LCA
    // Local, global one word loads and stores
    case 0x87: return 96;                              // LDL
    case 0x84: return 76;                              // LLA
    case 0xA4: return 96;                              // STL
    case 0x85: return 100;                             // LDO
    case 0x86: return 80;                              // LAO
    case 0xA5: return 132;                             // SRO
    // Intermediate: + 3.2 for each static link
    case 0x89: return 172 + 32 * a[0];                 // LOD
    case 0x88: return 152 + 32 * a[0];                 // LDA
    case 0xA6: return 168 + 32 * a[0];                 // STR
    case 0xC4: return 80;                              // STO
    // Extended
    case 0x9A: return 268;                             // LDE
    case 0x9B: return 248;                             // LAE
    case 0xD9: return 260;                             // STE
    // Multiple words
    case 0x83: return 180 + 40 * a[1];                 // LDC: 18.0 + 4.0(UB)
    case 0xD0: return 104 + 60 * a[0];                 // LDM: 10.4 + 6.0(UB)
    case 0x8E: return 124 + 60 * a[0];                 // STM: 12.4 + 6.0(UB)
    // Byte arrays
    case 0xA7: return 120;                             // LDB
    case 0xC8: return 136;                             // STB
    // Records and arrays
    case 0xC5: return 132 + 60 * a[0];                 // MOV: 13.2 + 6.0(B)
    case 0xE6: return 124;                             // IND
    case 0xE7: return 96;                              // INC
    case 0xD7:                                         // IXA: 9.6 at index 0
        return S(0) == 0 ? 96 : ixaTime10(a[0], I(0)); // no formula: measured, see above
    case 0xD8: {                                       // IXP
        // 27.6 for the first element of a word. The worst time per elements-per-word, at
        // the index range printed. 73.6 more if the index is not in the first word.
        int32_t epw = a[0], inx = I(0);
        if (epw == 0) return 276;                      // the core stops; no time printed
        int32_t p = std::abs(inx % epw);
        uint32_t worst = 0, worstFrom = 0;
        switch (epw) {
        case 3: worst = 372; worstFrom = 2; break;
        case 4: worst = 388; worstFrom = 3; break;
        case 5: worst = 396; worstFrom = 4; break;
        case 8: worst = 380; worstFrom = 3; break;
        case 16: worst = 356; worstFrom = 2; break;
        default: break;
        }
        uint32_t t;
        if (p == 0) t = 276;
        else if (worst && (uint32_t)p >= worstFrom) t = worst;
        else if (worst) t = mid(276, worst);           // no formula between best and worst
        else t = mid(276, 396);                        // no row for this elements-per-word
        return inx / epw != 0 ? t + 736 : t;
    }
    case 0xC9: {                                       // LDP: 18.4 + 2.0(width) + 2.0(start)
        uint32_t start = S(0) & 0xF, width = S(1) > 16 ? 16 : S(1);
        return 184 + 20 * width + 20 * start;
    }
    case 0xCA: {                                       // STP: 20.4 + 2.0(width) + 2.8(start)
        uint32_t start = S(1) & 0xF, width = S(2) > 16 ? 16 : S(2);
        return 204 + 20 * width + 28 * start;
    }
    // Logicals
    case 0xA1: case 0xA0: return 80;                   // LAND, LOR
    case 0xE5: return 52;                              // LNOT
    case 0x9F: return 60;                              // BNOT
    case 0xB4: return S(1) <= S(0) ? 96 : 104;         // LEUSW: TRUE | FALSE
    case 0xB5: return S(1) >= S(0) ? 96 : 104;         // GEUSW
    // Integers
    case 0xE0: return I(0) < 0 ? 60 : 48;              // ABI: neg | pos
    case 0xE1: return 52;                              // NGI
    case 0xE2: return 52;                              // DUP1
    case 0xA2: case 0xA3: return 80;                   // ADI, SBI
    case 0x8C:                                         // MPI: 5.2 for n * 0
        return (S(0) == 0 || S(1) == 0) ? 52 : 280;    // no formula: the printed typical 28.0
    case 0x8D: {                                       // DVI: 0 div i | pos | neg result
        int16_t y = I(0), x = I(1);
        if (x == 0 || y == 0) return 84;               // y = 0 raises: the shortest time
        return ((x < 0) != (y < 0)) ? 912 : 892;
    }
    case 0x8F: {                                       // MODI: 0 mod i | pos | neg result
        int16_t y = I(0), x = I(1);
        if (x == 0 || y <= 0) return 92;               // y <= 0 raises: the shortest time
        return x < 0 ? 928 : 892;                      // M0015 "negative": the x < 0 path
    }
    case 0xCB: return 144;                             // CHK
    case 0xB0: return I(1) == I(0) ? 96 : 104;         // EQUI: TRUE | FALSE
    case 0xB1: return I(1) != I(0) ? 96 : 104;         // NEQI
    case 0xB2: return I(1) <= I(0) ? 104 : 112;        // LEQI
    case 0xB3: return I(1) >= I(0) ? 104 : 112;        // GEQI
    // Reals
    case 0xCC: {                                       // FLT
        int16_t v = I(0);
        if (v == 0) return 108;
        int e = lg((float)v);
        return (uint32_t)(448 - 20 * e + (v < 0 ? 20 : 0));
    }
    case 0xBE: case 0xBF: {                            // TNC, RND
        float f = getReal(sp_);
        float m = std::fabs(f);
        uint32_t c = f < 0 ? 8 : 0;
        bool rnd = op == 0xBF;
        if (f == 0) return 124;
        if (std::isnan(f)) return 160;                 // no printed time; it raises 12
        if (m < 0.5f) return 156;
        if (m < 1.0f) return (rnd ? 524 : 500) + c;
        int e = (!std::isfinite(m) || m >= 3.4e38f) ? 127 : lg(m);  // NaN: no ilogb
        int32_t t = (rnd ? 488 : 480) - 8 * e + (int32_t)c;
        return t > 0 ? (uint32_t)t : 4;
    }
    case 0xE3: case 0xE4: return 52;                   // ABR, NGR
    case 0xC6: return 120;                             // DUP2
    case 0xC0:                                         // ADR: 18.8 if TOS-1 = 0.0
        return getReal((uint16_t)(sp_ + 2)) == 0 ? 188 : mid(608, 1528);  // no formula
    case 0xC1:                                         // SBR: 19.2 if TOS-1 = 0.0
        return getReal((uint16_t)(sp_ + 2)) == 0 ? 192 : mid(644, 1520);  // no formula
    case 0xC2:                                         // MPR: 26.4 if TOS-1 = 0.0
        return getReal((uint16_t)(sp_ + 2)) == 0 ? 264 : mid(1594, 1778); // no formula
    case 0xC3:                                         // DVR: 32.4 if TOS = 0.0 (it raises)
        return getReal(sp_) == 0 ? 324 : mid(1406, 2958);                 // no formula
    case 0xCD: case 0xCE: case 0xCF: {                 // EQUREAL, LEQREAL, GEQREAL
        float y = getReal(sp_), x = getReal((uint16_t)(sp_ + 2));
        bool hiEq = S(0) == S(2);
        if (op == 0xCD) return x == y ? 164 : !hiEq ? 148 : 184;
        if (x == y) return 164;
        // B: both operands positive ("pos < pos"); C: equal in the first (high) word.
        bool pos = !(S(0) & 0x8000) && !(S(2) & 0x8000);
        uint32_t c = hiEq ? 36 : 0;
        bool v = op == 0xCE ? x <= y : x >= y;
        if (op == 0xCE) return v ? 160 + (pos ? 8 : 0) + c : 168 + (pos ? 16 : 0) + c;
        return 160 + (pos ? 8 : 0) + c;                // GEQREAL: TRUE and FALSE the same
    }
    // Sets: words(TOS) = len0, words(TOS-1) = len1
    case 0xC7: {                                       // ADJ UB
        int32_t len = S(0), ub = a[0];
        if (len == ub) return 144;
        if (len < ub) return 136 + 68 * ub;            // expansion: 13.6 + 6.8(UB)
        // Compression, as printed: 16.4 + 5.6(words(TOS)) + 2.8(UB - words(TOS)).
        return (uint32_t)(164 + 56 * len + 28 * (ub - len));
    }
    case 0xBC: {                                       // SRS
        int32_t hi = I(0), lo = I(1);
        if (lo > hi || hi < 0 || lo < 0 || hi > 4079 || lo > 4079) return 180;  // 18.0: null
        int32_t len = hi / 16 + 1;
        if (len == 1) return (uint32_t)(504 + 20 * lo + 20 * hi);
        if (len == 2) return (uint32_t)(524 + 20 * (hi % 16) + 20 * (lo % 16) + (hi > 15 ? 16 : 0));
        uint32_t b = (hi / 16 - lo / 16) < 2 ? 4 : 0;
        return (uint32_t)(456 + 36 * len + 20 * (hi % 16) + 20 * (lo % 16)) - b;
    }
    case 0xDA: {                                       // INN
        int32_t len = S(0), v = (int16_t)S(len + 1);
        if (v < 0 || v > len * 16 - 1) return 184;     // outside the set's words
        return (uint32_t)(228 + 20 * (v % 16));
    }
    case 0xDB: {                                       // UNI
        int32_t len0 = S(0), len1 = S(len0 + 1);
        if (len0 == 0) return 66;
        if (len1 == 0) return (uint32_t)(224 + 68 * len0);
        if (len0 <= len1) return (uint32_t)(124 + 72 * len0);
        return (uint32_t)(240 + 140 * len0 + 68 * (len0 - len1));
    }
    case 0xDC: {                                       // INT
        int32_t len0 = S(0), len1 = S(len0 + 1);
        if (len0 == 0 && len1 == 0) return 116;
        if (len0 == 0) return 120;
        if (len0 >= len1) return (uint32_t)(152 + 72 * len0);
        return (uint32_t)(166 + 72 * len0 + 28 * (len1 - len0));
    }
    case 0xDD: {                                       // DIF
        int32_t len0 = S(0), len1 = S(len0 + 1);
        if (len0 == 0) return 60;
        if (len1 == 0) return 120;
        if (len0 <= len1) return (uint32_t)(140 + 72 * len0);
        return (uint32_t)(136 + 72 * len1);
    }
    case 0xB6: case 0xB7: case 0xB8: {                 // EQUPWR, LEQPWR, GEQPWR
        int32_t len0 = S(0), len1 = S(len0 + 1);
        PwrScan r = scanPwr(op);
        if (op == 0xB6) {
            uint32_t c = (r.d != 0 && r.v) ? 20 : 0;
            uint32_t b = len0 >= len1 ? 0 : r.v ? 12 : 8;
            return (uint32_t)(160 + 76 * r.n + 40 * r.d) + c + b;
        }
        if (op == 0xB7) {
            if (len0 >= len1) return (uint32_t)(160 + 84 * r.n);
            return (uint32_t)(172 + 84 * r.n + 40 * r.d) + ((r.d != 0 && r.v) ? 4 : 0);
        }
        if (len1 >= len0) {
            uint32_t c = r.v ? 12 : 0;
            uint32_t b = len0 == len1 ? 0 : r.v ? 4 : 12;
            return (uint32_t)(216 + 84 * r.n) + c + b;
        }
        return (uint32_t)(208 + 84 * r.n + 40 * r.d) + ((r.d != 0 && r.v) ? 20 : 0);
    }
    // Byte-array compares: B = the length, L/G/D = the bytes compared to decide
    case 0xB9: case 0xBA: case 0xBB: {
        ByteScan r = scanBytes(a[0]);
        if (r.cmp == 0) return (op == 0xB9 ? 192 : 184) + bytePasses(a[0]);
        if (op == 0xB9) return 114 + bytePasses(r.n);  // FALSE
        bool less = r.cmp < 0;
        if (op == 0xBA) return (less ? 168 : 176) + bytePasses(r.n);
        return (less ? 220 : 212) + bytePasses(r.n);
    }
    // Jumps: jump | no jump
    case 0x8A: return 124;                             // UJP
    case 0x8B: return 128;                             // UJPL
    case 0xD4: return (S(0) & 1) == 0 ? 168 : 108;     // FJP
    case 0xD5: return (S(0) & 1) == 0 ? 188 : 100;     // FJPL
    case 0xD2: return S(1) != S(0) ? 192 : 118;        // EFJ
    case 0xD3: return S(1) == S(0) ? 192 : 120;        // NFJ
    case 0xD6: {                                       // XJP
        uint16_t tbl = (uint16_t)(segb_ + a[0]);
        int16_t v = I(0), lo = (int16_t)rd(tbl), hi = (int16_t)rd((uint16_t)(tbl + 1));
        return v < lo ? 292 : v > hi ? 340 : 320;
    }
    // Calls and returns
    case 0x90: return 456;                             // CPL
    case 0x91: return 448;                             // CPG
    case 0x92: return 536 + 32 * a[0];                 // CPI: 53.6 + 3.2(DB)
    case 0x93: return 644;                             // CXL
    case 0x94: return 632;                             // CXG
    case 0x95: return 732 + 32 * a[1];                 // CXI: 73.2 + 3.2(DB)
    case 0x97: return 756;                             // CPF
    case 0x96: return 260;                             // RPU
    case 0x99: return 124 + 32 * a[0];                 // LSL: 12.4 + 3.2(DB)
    // System control
    case 0xDE: return signalTime10(S(0));              // SIGNAL
    case 0xDF: return rd(S(0)) > 0 ? 116 : 908;        // WAIT: no wait | task switch
    case 0x9D: return I(0) < 0 ? 84 : 552;             // LPR
    case 0xD1: {                                       // SPR
        int16_t r = I(1);
        return r >= 0 ? 548 : r == -1 ? 532 : 84;
    }
    case 0x9E: return 0;                               // BPT: only the Raise's time
    case 0x9C: return 36;                              // NOP
    case 0xBD: return 124;                             // SWAP
    default: return 0;
    }
}

} // namespace altair
