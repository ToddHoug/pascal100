// The `pcode` disassembler: WD III.0 P-code, for the Pascal MicroEngine core.
//
// Decoding comes from the one operator table in cpu/pcodeOps.h, so DISASM, the core and
// the tests cannot disagree about which opcode takes which parameters.
//
// There is no pcode Assembler, by design, as on the real system: everything was written
// in Pascal and the compiler made the P-code (C-1: no P-code assembler was provided).
//
// Addresses here are BYTE addresses in the code stream, the way the core's IPC counts
// them (a byte offset from SEGB, reference 7.1). A jump prints its TARGET, worked out
// from the next instruction (reference 7.3), the same way the 8080 decoder prints a
// JMP's address rather than its encoding.

#include "isa/isa.h"
#include "cpu/pcodeOps.h"

#include <cstdio>

namespace altair {

namespace {

// Same rule as the 8080 decoder's fmtNum (isa8080.cpp), kept file-local like isa8085.cpp
// and isaZ80.cpp do: `digits` is the hex width, octal shows a byte as 3 digits and a
// word as split octal.
std::string fmtNum(unsigned v, int digits, int base) {
    char b[16];
    if (base == 8) {
        if (digits <= 2)
            std::snprintf(b, sizeof b, "%03o", v & 0xFF);
        else
            std::snprintf(b, sizeof b, "%03o %03o", (v >> 8) & 0xFF, v & 0xFF);
        return b;
    }
    static const char* d = "0123456789ABCDEF";
    std::string s;
    for (int i = digits - 1; i >= 0; --i) s += d[(v >> (i * 4)) & 0xF];
    return s;
}

bool isJump(uint8_t op) {
    // The operators whose one SB or W parameter is a jump offset (C-1 6.6): UJP 8A, UJPL 8B,
    // EFJ D2, NFJ D3, FJP D4, FJPL D5. XJP's B is an offset into the constant block, not a
    // jump offset, so it prints as a number.
    return op == 0x8A || op == 0x8B || (op >= 0xD2 && op <= 0xD5);
}

class IsaPcode : public Disassembler {
public:
    const char* name() const override { return "pcode"; }

    Insn at(uint16_t addr, const PeekFn& peek, int base) const override {
        pcode::Decoded d = pcode::decode(addr, [&](uint32_t a) { return peek((uint16_t)a); });
        Insn in;
        in.len = d.len;
        if (!d.info) {
            // Unassigned (C-1 6.10). One byte, DDT's `??=` marker, as the 8080 decoder does.
            in.text = "?\?= " + fmtNum(d.op, 2, base);
            return in;
        }
        in.text = d.info->mnem;
        if (d.info->isShort()) {
            in.text += " " + fmtNum((unsigned)d.args[0], 2, base);
            return in;
        }
        if (isJump(d.op)) {
            uint16_t target = (uint16_t)(addr + d.len + d.args[0]);
            in.text += " " + fmtNum(target, 4, base);
            return in;
        }
        for (int i = 0; i < d.nargs; ++i) {
            pcode::Par k = d.info->par[i];
            unsigned v = (unsigned)d.args[i] & 0xFFFF;
            int digits = (k == pcode::Par::W || v > 0xFF) ? 4 : 2;
            in.text += (i == 0 ? " " : ",") + fmtNum(v, digits, base);
        }
        return in;
    }
};

const IsaPcode kPcode;

} // namespace

// The 8080 file owns the registry (disassemblerFor/instructionSets).
const Disassembler* pcodeDisassembler() { return &kPcode; }

} // namespace altair
