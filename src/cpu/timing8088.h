#pragma once
//
// 8088 INSTRUCTION TIMING, in T-states (clocks), for cpu8088.cpp.
//
// The CLK x86 core counts no cycles, and pascal100's cores return T-states per
// instruction, so this table supplies them. How it was made (2026-11-13; the work is in
// the project folder, Pascal100-Altairsim/clk-8088-spike/README.md):
//
//   1. MAME's i8086 timing and EA tables (src/devices/cpu/i86/i86.cpp, BSD-3-Clause,
//      copyright-holders Carl), applied to the raw bytes (prefixes, opcode, ModRM) and to
//      what the instruction did: branch taken, REP count, CL, word transfers.
//   2. The 8088 moves a word in two bus cycles: 4 clocks per word transfer. The core
//      counts its word transfers exactly.
//   3. REP string ops refitted to the real chip: about 12 + n x (MOVSB 17, CMPSB 22,
//      STOSB 10, LODSB 13, SCASB 15); the word forms follow from rule 2.
//   4. timing8088_adj.h: round(mean(real - table)) per opcode, ModRM reg and memory form,
//      from the SingleStepTests 8088 v2 cycle counts (a real 8088; MIT licence).
//
// Against those counts, from a full prefetch queue: mean abs error 0.8 clocks, 97.7%
// within +-2. The prefetch queue itself is not modelled; cpu8088.cpp adds a fetch floor
// (an instruction takes at least 4 clocks per byte of its own code), which puts us midway
// between the full-queue and empty-queue times (about 2 clocks either side).
#include <cstdint>
#include "cpu/timing8088_adj.h"

namespace altair::i8088 {

struct TimingFacts {
    bool repFirst = true;     // REP: the first iteration of this instruction (pays the base)
    bool taken = false;       // Jcc / LOOPx / JCXZ / INTO took the branch
    int reps = 0;             // REP string op: iterations done (CX before - after)
    int shiftCount = 0;       // D2/D3: CL as it was
    int wordTransfers = 0;    // memory/I/O word accesses the instruction made (from the bus)
};

// MAME's 8086 EA times, by mod (0-2) and rm.
static const uint8_t kEa[3][8] = {
    {7, 8, 8, 7, 5, 5, 6, 5},
    {11, 12, 12, 11, 9, 9, 9, 9},
    {11, 12, 11, 11, 9, 9, 9, 9},
};

inline int timing(const uint8_t* b, int len, const TimingFacts& f) {
    int t = 0, i = 0;
    bool rep = false;
    // prefixes
    for (; i < len; ++i) {
        uint8_t p = b[i];
        if (p == 0x26 || p == 0x2E || p == 0x36 || p == 0x3E) t += 2;      // segment override
        else if (p == 0xF2 || p == 0xF3) rep = true;
        else if (p == 0xF0) t += 2;                                          // LOCK
        else break;
    }
    const uint8_t op = b[i];
    const uint8_t modrm = (i + 1 < len) ? b[i + 1] : 0;
    const int mod = modrm >> 6, rm = modrm & 7, reg = (modrm >> 3) & 7;
    const bool mem = mod != 3;
    const int ea = mem ? kEa[mod][rm] : 0;
    const bool w = op & 1;

    auto alu = [&](bool toReg) {           // 00-3F reg/mem forms
        if (!mem) return 3;
        return toReg ? 9 + ea : 16 + ea;  // ALU_RM (reg <- mem) 9, ALU_MR (mem <- reg) 16
    };

    switch (op) {
    // ---- ALU, opcodes 00-3F ----------------------------------------------------------
    case 0x00: case 0x01: case 0x08: case 0x09: case 0x10: case 0x11: case 0x18: case 0x19:
    case 0x20: case 0x21: case 0x28: case 0x29: case 0x30: case 0x31:
        t += alu(false); break;
    case 0x38: case 0x39:  // CMP mem,reg reads only
        t += mem ? 9 + ea : 3; break;
    case 0x02: case 0x03: case 0x0A: case 0x0B: case 0x12: case 0x13: case 0x1A: case 0x1B:
    case 0x22: case 0x23: case 0x2A: case 0x2B: case 0x32: case 0x33: case 0x3A: case 0x3B:
        t += alu(true); break;
    case 0x04: case 0x05: case 0x0C: case 0x0D: case 0x14: case 0x15: case 0x1C: case 0x1D:
    case 0x24: case 0x25: case 0x2C: case 0x2D: case 0x34: case 0x35: case 0x3C: case 0x3D:
        t += 4; break;  // ALU acc,imm
    case 0x06: case 0x0E: case 0x16: case 0x1E: t += 14; break;  // PUSH seg
    case 0x07: case 0x0F: case 0x17: case 0x1F: t += 12; break;  // POP seg
    case 0x27: case 0x2F: t += 4; break;     // DAA DAS
    case 0x37: case 0x3F: t += 4; break;     // AAA AAS
    // ---- 40-5F ------------------------------------------------------------------------
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: t += 2; break;       // INC/DEC r16 (MAME INCDEC_R16 = 2)
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: t += 15; break;      // PUSH r16
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: t += 12; break;      // POP r16
    // ---- Jcc (60-6F alias 70-7F on the 8088) ------------------------------------------
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: t += f.taken ? 16 : 4; break;
    // ---- 80-83 group 1 ----------------------------------------------------------------
    case 0x80: case 0x81: case 0x82: case 0x83: {
        const bool cmp = reg == 7;
        if (!mem) t += 4;
        else t += (cmp ? 10 : 17) + ea;     // ALU_MI_RO (CMP) 10, ALU_MI 17
        break;
    }
    case 0x84: case 0x85: t += mem ? 9 + ea : 3; break;            // TEST
    case 0x86: case 0x87: t += mem ? 17 + ea : 4; break;           // XCHG
    case 0x88: case 0x89: t += mem ? 9 + ea : 2; break;            // MOV mem,reg
    case 0x8A: case 0x8B: t += mem ? 8 + ea : 2; break;            // MOV reg,mem
    case 0x8C: t += mem ? 9 + ea : 2; break;                       // MOV r/m,seg
    case 0x8D: t += 2 + ea; break;                                 // LEA
    case 0x8E: t += mem ? 8 + ea : 2; break;                       // MOV seg,r/m
    case 0x8F: t += mem ? 25 + ea : 12; break;                     // POP r/m
    case 0x90: t += 3; break;                                      // NOP
    case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: t += 3; break;                             // XCHG AX,r16
    case 0x98: t += 2; break;                                      // CBW
    case 0x99: t += 5; break;                                      // CWD
    case 0x9A: t += 28; break;                                     // CALL far
    case 0x9B: t += 3; break;                                      // WAIT
    case 0x9C: t += 14; break;                                     // PUSHF
    case 0x9D: t += 12; break;                                     // POPF
    case 0x9E: case 0x9F: t += 4; break;                           // SAHF LAHF
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: t += 10; break;    // MOV acc<->moffs
    case 0xA4: case 0xA5: t += rep ? (f.repFirst ? 12 : 0) + 17 * f.reps : 18; break; // MOVS (REP fit: 12 + 17n)
    case 0xA6: case 0xA7: t += rep ? (f.repFirst ? 11 : 0) + 22 * f.reps : 22; break; // CMPS (REP fit: 11 + 22n)
    case 0xA8: case 0xA9: t += 4; break;                           // TEST acc,imm
    case 0xAA: case 0xAB: t += rep ? (f.repFirst ? 12 : 0) + 10 * f.reps : 11; break; // STOS (REP fit: 12 + 10n)
    case 0xAC: case 0xAD: t += rep ? (f.repFirst ? 12 : 0) + 13 * f.reps : 12; break; // LODS (REP fit: 12 + 13n)
    case 0xAE: case 0xAF: t += rep ? (f.repFirst ? 11 : 0) + 15 * f.reps : 15; break; // SCAS (REP fit: 11 + 15n)
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: t += 4; break;                             // MOV r,imm
    case 0xC0: case 0xC2: t += 24; break;                          // RET n (C0 aliases C2)
    case 0xC1: case 0xC3: t += 20; break;                          // RET
    case 0xC4: case 0xC5: t += 24 + ea; break;                     // LES LDS
    case 0xC6: case 0xC7: t += mem ? 10 + ea : 4; break;           // MOV r/m,imm
    case 0xC8: case 0xCA: t += 31; break;                          // RETF n
    case 0xC9: case 0xCB: t += 32; break;                          // RETF
    case 0xCC: t += 53; break;                                     // INT 3
    case 0xCD: t += 51; break;                                     // INT n
    case 0xCE: t += f.taken ? 53 : 4; break;                       // INTO
    case 0xCF: t += 32; break;                                     // IRET
    case 0xD0: case 0xD1: t += mem ? 15 + ea : 2; break;           // shift by 1
    case 0xD2: case 0xD3: t += (mem ? 20 + ea : 8) + 4 * f.shiftCount; break;
    case 0xD4: t += 83; break;                                     // AAM
    case 0xD5: t += 60; break;                                     // AAD
    case 0xD6: t += 4; break;                                      // SALC (no source; guess)
    case 0xD7: t += 11; break;                                     // XLAT
    case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF: t += mem ? 8 + ea : 2; break;              // ESC (no source; guess)
    case 0xE0: t += f.taken ? 18 : 6; break;                       // LOOPNE
    case 0xE1: t += f.taken ? 18 : 6; break;                       // LOOPE
    case 0xE2: t += f.taken ? 17 : 5; break;                       // LOOP
    case 0xE3: t += f.taken ? 18 : 6; break;                       // JCXZ
    case 0xE4: case 0xE5: t += 10; break;                          // IN imm
    case 0xE6: case 0xE7: t += 10; break;                          // OUT imm
    case 0xE8: t += 19; break;                                     // CALL near
    case 0xE9: case 0xEB: t += 15; break;                          // JMP near/short
    case 0xEA: t += 15; break;                                     // JMP far
    case 0xEC: case 0xED: t += 8; break;                           // IN DX
    case 0xEE: case 0xEF: t += 8; break;                           // OUT DX
    case 0xF1: t += 2; break;                                      // alias LOCK (guess)
    case 0xF4: t += 2; break;                                      // HLT
    case 0xF5: case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD:
        t += 2; break;                                             // flag ops
    case 0xF6: case 0xF7:
        switch (reg) {
        case 0: case 1: t += mem ? 10 + ea : 4; break;             // TEST r/m,imm (MAME ALU_MI_RO)
        case 2: case 3: t += mem ? 16 + ea : 3; break;             // NOT NEG
        case 4: t += mem ? (w ? 128 : 76) + ea : (w ? 118 : 70); break;   // MUL
        case 5: t += mem ? (w ? 138 : 86) + ea : (w ? 128 : 80); break;   // IMUL
        case 6: t += mem ? (w ? 154 : 86) + ea : (w ? 144 : 80); break;   // DIV
        case 7: t += mem ? (w ? 175 : 107) + ea : (w ? 165 : 101); break; // IDIV
        }
        break;
    case 0xFE: case 0xFF:
        switch (reg) {
        case 0: case 1: t += mem ? 15 + ea : (w ? 2 : 3); break;   // INC DEC
        case 2: t += mem ? 21 + ea : 16; break;                    // CALL near indirect
        case 3: t += 37 + ea; break;                               // CALL far indirect
        case 4: t += mem ? 18 + ea : 11; break;                    // JMP near indirect
        case 5: t += 24 + ea; break;                               // JMP far indirect
        case 6: case 7: t += mem ? 24 + ea : 15; break;            // PUSH r/m
        }
        break;
    default: t += 2; break;
    }
    t += kAdj[op][(op == 0x80 || op == 0x81 || op == 0x82 || op == 0x83 || (op >= 0xD0 && op <= 0xD3) ||
                  op == 0xF6 || op == 0xF7 || op == 0xFE || op == 0xFF) ? reg : 0][mem ? 1 : 0];
    return t + 4 * f.wordTransfers;  // the 8088 moves a word in two bus cycles
}

} // namespace altair::i8088
