#pragma once
//
// THE WD III.0 P-CODE OPERATOR TABLE -- one table, four readers.
//
// The P-machine core decodes with it, the `pcode` disassembler prints with it, the
// tests encode with it, and the coverage test counts against it. A second copy of
// "which opcode takes which parameters" in any of them would be a second place for a
// transcription slip to hide, so there is none.
//
// Source: Table C-1 of WD's *UCSD Pascal III.0 Operating System Reference Manual*
// (Jul 1982), transcribed in reference/80-0130016-00A0_..._Jul82.md, section 6.
// 100 operators on 168 of the 256 byte values. Unassigned: 40..77, A8..AF, E8..FF.
// Opcode 9F is BNOT, not the 1979 list's RBP (reference section 7.11).

#include <cstdint>
#include <string>
#include <vector>

namespace altair::pcode {

// Parameter kinds (C-1, reference section 2):
//   UB  unsigned byte            SB  signed byte (jump offsets)
//   DB  "don't care" byte, 0..127 (static link counts)
//   B   "big": one byte 0..127, or two bytes with bit 7 of the first set and the
//       value's high 7 bits in it, then the low byte (128..32767)
//   W   a word, low byte first
enum class Par : uint8_t { UB, SB, DB, B, W };

struct OpInfo {
    const char* mnem;   // "LDL"; a short form's base name: "SLDC", "SLDL", "SLDO", "SIND"
    uint8_t first;      // the opcode, or the first of a short form's range
    uint8_t last;       // == first, except for the short forms
    uint8_t shortBase;  // a short form's value at `first`: SLDC0, SLDL1, SLDO1, SIND0
    uint8_t npar;
    Par par[3];

    bool isShort() const { return first != last; }
    // The short form's built-in value for opcode `op` (SLDC 5 -> 5, SLDL 1 -> 1).
    int shortValue(uint8_t op) const { return op - first + shortBase; }
};

// Every row of C-1, in opcode order.
const std::vector<OpInfo>& opTable();

// The row for an opcode, or nullptr if the opcode is unassigned.
const OpInfo* opInfo(uint8_t op);

// One decoded instruction. `args` are the parameter VALUES (a B is already widened, an SB
// and a W are sign-extended); a short form has its built-in value as args[0], nargs 1.
struct Decoded {
    const OpInfo* info = nullptr;  // nullptr: unassigned opcode
    uint8_t op = 0;
    uint8_t len = 1;               // bytes, opcode included
    uint8_t nargs = 0;
    int32_t args[3] = {0, 0, 0};
};

// Decode the instruction whose opcode is the byte at `at`. `byteAt` reads one code byte.
template <class ByteAt>
Decoded decode(uint32_t at, ByteAt&& byteAt) {
    Decoded d;
    d.op = byteAt(at);
    d.info = opInfo(d.op);
    if (!d.info) return d;
    if (d.info->isShort()) {
        d.nargs = 1;
        d.args[0] = d.info->shortValue(d.op);
        return d;
    }
    uint32_t p = at + 1;
    for (int i = 0; i < d.info->npar; ++i) {
        uint8_t b0 = byteAt(p++);
        int32_t v = b0;
        switch (d.info->par[i]) {
        case Par::UB: case Par::DB: break;
        case Par::SB: v = (int8_t)b0; break;
        case Par::B:
            if (b0 & 0x80) v = ((b0 & 0x7F) << 8) | byteAt(p++);
            break;
        case Par::W: v = (int16_t)(b0 | (byteAt(p++) << 8)); break;
        }
        d.args[d.nargs++] = v;
    }
    d.len = (uint8_t)(p - at);
    return d;
}

// Encode one instruction. `mnem` is case-blind; a short form is its base name plus its
// value as the one argument ("SLDC", {5}). Returns the bytes, or empty with `err` set.
// Jump offsets are encoded as given (relative to the NEXT instruction, reference 7.3).
std::vector<uint8_t> encode(const std::string& mnem, const std::vector<int32_t>& args,
                            std::string& err);

} // namespace altair::pcode
