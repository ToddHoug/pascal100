// The WD III.0 P-code operator table (see pcodeOps.h).
#include "cpu/pcodeOps.h"

#include <array>
#include <cctype>

namespace altair::pcode {

namespace {
using P = Par;
}

const std::vector<OpInfo>& opTable() {
    // Generated from reference/80-0130016-00A0_..._Jul82.md section 6 (Table C-1), RBP
    // left out (reference 7.11). UB1/UB2 are both UB. Opcode order.
    static const std::vector<OpInfo> t = {
        {"SLDC", 0x00, 0x1F, 0, 0, {}},
        {"SLDL", 0x20, 0x2F, 1, 0, {}},
        {"SLDO", 0x30, 0x3F, 1, 0, {}},
        {"SIND", 0x78, 0x7F, 0, 0, {}},
        {"LDCB", 0x80, 0x80, 0, 1, {P::UB}},
        {"LDCI", 0x81, 0x81, 0, 1, {P::W}},
        {"LCA", 0x82, 0x82, 0, 1, {P::B}},
        {"LDC", 0x83, 0x83, 0, 2, {P::B, P::UB}},
        {"LLA", 0x84, 0x84, 0, 1, {P::B}},
        {"LDO", 0x85, 0x85, 0, 1, {P::B}},
        {"LAO", 0x86, 0x86, 0, 1, {P::B}},
        {"LDL", 0x87, 0x87, 0, 1, {P::B}},
        {"LDA", 0x88, 0x88, 0, 2, {P::DB, P::B}},
        {"LOD", 0x89, 0x89, 0, 2, {P::DB, P::B}},
        {"UJP", 0x8A, 0x8A, 0, 1, {P::SB}},
        {"UJPL", 0x8B, 0x8B, 0, 1, {P::W}},
        {"MPI", 0x8C, 0x8C, 0, 0, {}},
        {"DVI", 0x8D, 0x8D, 0, 0, {}},
        {"STM", 0x8E, 0x8E, 0, 1, {P::UB}},
        {"MODI", 0x8F, 0x8F, 0, 0, {}},
        {"CPL", 0x90, 0x90, 0, 1, {P::UB}},
        {"CPG", 0x91, 0x91, 0, 1, {P::UB}},
        {"CPI", 0x92, 0x92, 0, 2, {P::DB, P::UB}},
        {"CXL", 0x93, 0x93, 0, 2, {P::UB, P::UB}},
        {"CXG", 0x94, 0x94, 0, 2, {P::UB, P::UB}},
        {"CXI", 0x95, 0x95, 0, 3, {P::UB, P::DB, P::UB}},
        {"RPU", 0x96, 0x96, 0, 1, {P::B}},
        {"CPF", 0x97, 0x97, 0, 0, {}},
        {"LDCN", 0x98, 0x98, 0, 0, {}},
        {"LSL", 0x99, 0x99, 0, 1, {P::DB}},
        {"LDE", 0x9A, 0x9A, 0, 2, {P::UB, P::B}},
        {"LAE", 0x9B, 0x9B, 0, 2, {P::UB, P::B}},
        {"NOP", 0x9C, 0x9C, 0, 0, {}},
        {"LPR", 0x9D, 0x9D, 0, 0, {}},
        {"BPT", 0x9E, 0x9E, 0, 0, {}},
        {"BNOT", 0x9F, 0x9F, 0, 0, {}},
        {"LOR", 0xA0, 0xA0, 0, 0, {}},
        {"LAND", 0xA1, 0xA1, 0, 0, {}},
        {"ADI", 0xA2, 0xA2, 0, 0, {}},
        {"SBI", 0xA3, 0xA3, 0, 0, {}},
        {"STL", 0xA4, 0xA4, 0, 1, {P::B}},
        {"SRO", 0xA5, 0xA5, 0, 1, {P::B}},
        {"STR", 0xA6, 0xA6, 0, 2, {P::DB, P::B}},
        {"LDB", 0xA7, 0xA7, 0, 0, {}},
        {"EQUI", 0xB0, 0xB0, 0, 0, {}},
        {"NEQI", 0xB1, 0xB1, 0, 0, {}},
        {"LEQI", 0xB2, 0xB2, 0, 0, {}},
        {"GEQI", 0xB3, 0xB3, 0, 0, {}},
        {"LEUSW", 0xB4, 0xB4, 0, 0, {}},
        {"GEUSW", 0xB5, 0xB5, 0, 0, {}},
        {"EQUPWR", 0xB6, 0xB6, 0, 0, {}},
        {"LEQPWR", 0xB7, 0xB7, 0, 0, {}},
        {"GEQPWR", 0xB8, 0xB8, 0, 0, {}},
        {"EQUBYT", 0xB9, 0xB9, 0, 1, {P::B}},
        {"LEQBYT", 0xBA, 0xBA, 0, 1, {P::B}},
        {"GEQBYT", 0xBB, 0xBB, 0, 1, {P::B}},
        {"SRS", 0xBC, 0xBC, 0, 0, {}},
        {"SWAP", 0xBD, 0xBD, 0, 0, {}},
        {"TNC", 0xBE, 0xBE, 0, 0, {}},
        {"RND", 0xBF, 0xBF, 0, 0, {}},
        {"ADR", 0xC0, 0xC0, 0, 0, {}},
        {"SBR", 0xC1, 0xC1, 0, 0, {}},
        {"MPR", 0xC2, 0xC2, 0, 0, {}},
        {"DVR", 0xC3, 0xC3, 0, 0, {}},
        {"STO", 0xC4, 0xC4, 0, 0, {}},
        {"MOV", 0xC5, 0xC5, 0, 1, {P::B}},
        {"DUP2", 0xC6, 0xC6, 0, 0, {}},
        {"ADJ", 0xC7, 0xC7, 0, 1, {P::UB}},
        {"STB", 0xC8, 0xC8, 0, 0, {}},
        {"LDP", 0xC9, 0xC9, 0, 0, {}},
        {"STP", 0xCA, 0xCA, 0, 0, {}},
        {"CHK", 0xCB, 0xCB, 0, 0, {}},
        {"FLT", 0xCC, 0xCC, 0, 0, {}},
        {"EQUREAL", 0xCD, 0xCD, 0, 0, {}},
        {"LEQREAL", 0xCE, 0xCE, 0, 0, {}},
        {"GEQREAL", 0xCF, 0xCF, 0, 0, {}},
        {"LDM", 0xD0, 0xD0, 0, 1, {P::UB}},
        {"SPR", 0xD1, 0xD1, 0, 0, {}},
        {"EFJ", 0xD2, 0xD2, 0, 1, {P::SB}},
        {"NFJ", 0xD3, 0xD3, 0, 1, {P::SB}},
        {"FJP", 0xD4, 0xD4, 0, 1, {P::SB}},
        {"FJPL", 0xD5, 0xD5, 0, 1, {P::W}},
        {"XJP", 0xD6, 0xD6, 0, 1, {P::B}},
        {"IXA", 0xD7, 0xD7, 0, 1, {P::B}},
        {"IXP", 0xD8, 0xD8, 0, 2, {P::UB, P::UB}},
        {"STE", 0xD9, 0xD9, 0, 2, {P::UB, P::B}},
        {"INN", 0xDA, 0xDA, 0, 0, {}},
        {"UNI", 0xDB, 0xDB, 0, 0, {}},
        {"INT", 0xDC, 0xDC, 0, 0, {}},
        {"DIF", 0xDD, 0xDD, 0, 0, {}},
        {"SIGNAL", 0xDE, 0xDE, 0, 0, {}},
        {"WAIT", 0xDF, 0xDF, 0, 0, {}},
        {"ABI", 0xE0, 0xE0, 0, 0, {}},
        {"NGI", 0xE1, 0xE1, 0, 0, {}},
        {"DUP1", 0xE2, 0xE2, 0, 0, {}},
        {"ABR", 0xE3, 0xE3, 0, 0, {}},
        {"NGR", 0xE4, 0xE4, 0, 0, {}},
        {"LNOT", 0xE5, 0xE5, 0, 0, {}},
        {"IND", 0xE6, 0xE6, 0, 1, {P::B}},
        {"INC", 0xE7, 0xE7, 0, 1, {P::B}},
    };
    return t;
}

const OpInfo* opInfo(uint8_t op) {
    static const std::array<const OpInfo*, 256> byOp = [] {
        std::array<const OpInfo*, 256> a{};
        for (const OpInfo& r : opTable())
            for (int o = r.first; o <= r.last; ++o) a[o] = &r;
        return a;
    }();
    return byOp[op];
}

std::vector<uint8_t> encode(const std::string& mnem, const std::vector<int32_t>& args,
                            std::string& err) {
    std::string m;
    for (char c : mnem) m += (char)std::toupper((unsigned char)c);
    const OpInfo* info = nullptr;
    for (const OpInfo& r : opTable())
        if (m == r.mnem) { info = &r; break; }
    if (!info) { err = "unknown P-code operator: " + mnem; return {}; }

    if (info->isShort()) {
        int lo = info->shortBase, hi = info->shortBase + (info->last - info->first);
        if (args.size() != 1 || args[0] < lo || args[0] > hi) {
            err = m + " takes one value, " + std::to_string(lo) + ".." + std::to_string(hi);
            return {};
        }
        return {(uint8_t)(info->first + args[0] - lo)};
    }
    if (args.size() != info->npar) {
        err = m + " takes " + std::to_string(info->npar) + " parameter(s)";
        return {};
    }
    std::vector<uint8_t> out{info->first};
    for (int i = 0; i < info->npar; ++i) {
        int32_t v = args[i];
        auto range = [&](int32_t lo, int32_t hi) {
            if (v >= lo && v <= hi) return true;
            err = m + " parameter " + std::to_string(i + 1) + " out of range " +
                  std::to_string(lo) + ".." + std::to_string(hi);
            return false;
        };
        switch (info->par[i]) {
        case P::UB: if (!range(0, 255)) return {}; out.push_back((uint8_t)v); break;
        case P::DB: if (!range(0, 127)) return {}; out.push_back((uint8_t)v); break;
        case P::SB: if (!range(-128, 127)) return {}; out.push_back((uint8_t)v); break;
        case P::B:
            if (!range(0, 32767)) return {};
            if (v <= 127) out.push_back((uint8_t)v);
            else { out.push_back((uint8_t)(0x80 | (v >> 8))); out.push_back((uint8_t)v); }
            break;
        case P::W:
            if (!range(-32768, 65535)) return {};
            out.push_back((uint8_t)v);
            out.push_back((uint8_t)(v >> 8));
            break;
        }
    }
    return out;
}

} // namespace altair::pcode
