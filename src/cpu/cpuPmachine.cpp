// PMachineCore -- see cpuPmachine.h. Each operator below is a transcription of Table C-3
// (reference section 2), with the readings that section 7 settles. A `// 2.x` tag names the
// reference section; a `// 7.x` tag names the settled reading that differs from the print.

#include "cpu/cpuPmachine.h"

#include "core/statefile.h"
#include "cpu/pcodeOps.h"

#include <bit>
#include <cmath>
#include <cstdio>
#include <vector>

namespace altair {

namespace {

// Mark stack control word (C-34): msstat, msdynl, msipc, then msseg (low byte) and msflag.
constexpr uint16_t kMsStat = 0, kMsDynl = 1, kMsIpc = 2, kMsSeg = 3;
constexpr uint16_t kMscwSz = 4;

// TIB word offsets (C-34, the III.0 layout; register numbers of LPR/SPR, reference 7.8).
constexpr uint16_t kTibSplow = 2;
constexpr uint16_t kTibSp = 4, kTibMp = 5, kTibBp = 6, kTibIpc = 7, kTibSegb = 8;
constexpr uint16_t kTibSibs = 11;

// SIB word offsets (C-34).
constexpr uint16_t kSibSegbase = 0, kSibSegrefs = 2;

constexpr uint16_t b(bool v) { return v ? 1 : 0; }  // a Pascal BOOLEAN: bit 0 only (M0015)

// A Raise is a call of the error handler. M0015 gives BPT, which only raises, "comparable
// to the time for CXG": 63.2 us.
constexpr uint32_t kRaiseTime10 = 632;

std::string hex2(unsigned v) { char s[8]; std::snprintf(s, sizeof s, "%02X", v & 0xFF); return s; }
std::string hex4(unsigned v) { char s[8]; std::snprintf(s, sizeof s, "%04X", v & 0xFFFF); return s; }

} // namespace

std::vector<RegDef> PMachineCore::registers() {
    auto reg = [](const char* n, const char* help, uint16_t* p) {
        return RegDef{n, 16, "", RegShow::Field, help, [p] { return (uint32_t)*p; },
                      [p](uint32_t v) { *p = (uint16_t)v; }};
    };
    // Names as the Pascal-100 manual (F 3.7.2, 3.7.3) and LPR/SPR call them.
    return {
        reg("SP", "top-of-stack word address (the stack grows toward low memory)", &sp_),
        reg("MP", "mark stack control word of the running procedure", &mp_),
        reg("BP", "mark stack control word of the global (base) frame", &bp_),
        reg("SEGB", "word address of the running code segment", &segb_),
        reg("CTP", "current task: its TIB", &ctp_),
        reg("RQP", "ready queue: the first TIB", &rqp_),
        reg("SDP", "segment dictionary (system segment vector)", &sdp_),
        reg("IPC", "next instruction: a BYTE offset from SEGB", &ipc_),
    };
}

// The list above, as plain loads (CpuCore::captureRegs). SAME ORDER, entry for entry --
// tests/test_pmachine.cpp holds it to registers(). The history records every operator, and
// the default builds registers() each time: 38% of the run time in the FIT compile.
void PMachineCore::captureRegs(std::vector<uint32_t>& out) {
    out.resize(8);
    uint32_t* o = out.data();
    o[0] = sp_; o[1] = mp_; o[2] = bp_; o[3] = segb_;
    o[4] = ctp_; o[5] = rqp_; o[6] = sdp_; o[7] = ipc_;
}

void PMachineCore::reset(Reset) {
    // A core touches no memory at reset (cpu.h). The MicroEngine's microcode reads the boot
    // block when it starts to run, so the read happens on the first step (boot()).
    state_ = State::Start;
    stopReason_.clear();
    raising_ = false;
    t10_ = 0;
    carry10_ = 0;
}

void PMachineCore::stop(const std::string& why) {
    state_ = State::Stopped;
    stopReason_ = why;
}

// A stop inside an operator: IPC goes back to the operator, so pc() shows the instruction
// that stopped, as for an unassigned opcode. The operator has not changed the stack.
void PMachineCore::stopAt(uint16_t at, const std::string& why) {
    ipc_ = at;
    stop(why + " at SEGB " + hex4(segb_) + " IPC " + hex4(at));
}

// Pascal-100 manual F 3.7.2, the bootstrap flowchart, steps 2-3 and 16-20.
void PMachineCore::boot() {
    uint16_t mr = rd(kInitVector);
    if (mr == 0) {
        // The flowchart's MR = 0 branch is WD's own floppy boot through hardware at FC30.
        // The Pascal-100 has none: its owner's manual marks the ROM branch "Pascal-100".
        stop("boot: word FC68 is 0 (WD's floppy boot, not on the Pascal-100)");
        return;
    }
    ctp_ = rd(mr);
    sdp_ = rd((uint16_t)(mr + 1));
    rqp_ = rd((uint16_t)(mr + 2));
    sp_ = rd((uint16_t)(ctp_ + kTibSp));
    mp_ = rd((uint16_t)(ctp_ + kTibMp));
    bp_ = rd((uint16_t)(ctp_ + kTibBp));
    ipc_ = rd((uint16_t)(ctp_ + kTibIpc));
    segb_ = rd((uint16_t)(ctp_ + kTibSegb));
    state_ = State::Running;
}

StepResult PMachineCore::step(Bus&) { return step(); }

StepResult PMachineCore::step() {
    StepResult r;
    t10_ = 0;
    if (!port_) {
        stop("no memory attached");
    } else if (state_ == State::Start) {
        boot();
    } else if (state_ == State::Running) {
        execute();
    } else if (state_ == State::Idle) {
        dispatch();  // label 6 again: takes an interrupt if one is pending
    }
    if (state_ == State::Stopped || state_ == State::Idle) r.status = RunStatus::Halted;
    // M0015 gives times in microseconds at 2.5 MHz: one cycle is 0.4 us, 4 tenths. A few
    // printed times are not whole cycles (6.6, 159.4, ...), so the odd tenths carry to the
    // next step and the total stays exact. A step with no printed time (the boot, an idle
    // step, a stop) counts one cycle, so that a clock fed by these cycles still runs.
    if (t10_ == 0) {
        r.tStates = 1;
    } else {
        uint32_t t = t10_ + carry10_;
        r.tStates = t / 4;
        carry10_ = (uint8_t)(t % 4);
    }
    return r;
}

uint16_t PMachineCore::walkStatic(uint16_t from, int links) {
    uint16_t lm = from;
    for (int i = 0; i < links; ++i) lm = rd((uint16_t)(lm + kMsStat));
    return lm;
}

uint16_t PMachineCore::sibOf(uint16_t segno) {
    // 2.6 getsegb: segments 0..127 through ssv, 128..255 through the task's own SIB vector.
    if (segno < 128) return rd((uint16_t)(sdp_ + segno));
    uint16_t sibs = rd((uint16_t)(ctp_ + kTibSibs));
    return rd((uint16_t)(sibs + (segno - 128)));
}

uint16_t PMachineCore::getsegb(uint16_t segno) {
    return rd((uint16_t)(sibOf(segno) + kSibSegbase));
}

uint8_t PMachineCore::thisSegNo() {
    // 7.2: the word at segb + segb^.i holds the segment number in its low byte.
    return (uint8_t)rd((uint16_t)(segb_ + rd(segb_)));
}

float PMachineCore::getReal(uint16_t a) {
    uint32_t bits = ((uint32_t)rd(a) << 16) | rd((uint16_t)(a + 1));
    return std::bit_cast<float>(bits);
}

void PMachineCore::putReal(uint16_t a, float f) {
    uint32_t bits = std::bit_cast<uint32_t>(f);
    wr(a, (uint16_t)(bits >> 16));
    wr((uint16_t)(a + 1), (uint16_t)bits);
}

bool PMachineCore::realResult(float& r) {
    // Overflow raises the error. Underflow does NOT, whatever the manual says: on the real
    // machine a result below the smallest normal number is +0, for either sign (reference
    // 7.12). The rounding is the host's IEEE round-to-nearest, as the real machine does:
    // 1.0/3.0 is 3EAA AAAB there too (reference 7.12).
    if (!std::isfinite(r)) {
        raise(kFloatError);
        return false;
    }
    if (std::fabs(r) < 1.17549435e-38f) r = 0.0f;
    return true;
}

void PMachineCore::addSegrefs(uint16_t segno, int delta) {
    uint16_t sib = sibOf(segno);
    wr((uint16_t)(sib + kSibSegrefs), (uint16_t)(rd((uint16_t)(sib + kSibSegrefs)) + delta));
}

void PMachineCore::callExternal(uint16_t segno, uint16_t proc, uint16_t statLink) {
    // CXL, CXG, CXI, CPF (2.19): msseg is the CALLER's segment number, so RPU can return
    // to it; segb becomes the callee's; the callee's SIB counts the call. The segment is
    // found through getsegb, as for LDE (2.6, 2.19 notes).
    uint8_t caller = thisSegNo();
    if (call(getsegb(segno), proc, statLink, caller)) addSegrefs(segno, +1);
}

// ---- 2.21 tasks ----------------------------------------------------------------

void PMachineCore::updatetib() {
    wr((uint16_t)(ctp_ + kTibMp), mp_);
    wr((uint16_t)(ctp_ + kTibBp), bp_);
    wr((uint16_t)(ctp_ + kTibSp), sp_);
    wr((uint16_t)(ctp_ + kTibIpc), ipc_);
    wr((uint16_t)(ctp_ + kTibSegb), segb_);
}

void PMachineCore::loadtib() {
    sp_ = rd((uint16_t)(ctp_ + kTibSp));
    mp_ = rd((uint16_t)(ctp_ + kTibMp));
    bp_ = rd((uint16_t)(ctp_ + kTibBp));
    ipc_ = rd((uint16_t)(ctp_ + kTibIpc));
    segb_ = rd((uint16_t)(ctp_ + kTibSegb));
}

uint16_t PMachineCore::enque(uint16_t head, uint16_t task) {
    // Before the first task of LOWER priority, so equal priorities keep their order.
    uint16_t t1 = head, t2 = kNil;
    while (t1 != kNil && !(prior(t1) < prior(task))) {
        t2 = t1;
        t1 = rd(t1);
    }
    wr(task, t1);
    if (t2 == kNil) return task;
    wr(t2, task);
    return head;
}

void PMachineCore::taskswitch() {
    updatetib();
    dispatch();
}

void PMachineCore::dispatch() {
    if (rqp_ == kNil) {
        // No task is ready. An interrupt is the only way on: the card names the vector,
        // the vector points at the semaphore, and the P-machine signals it with no task.
        if (port_->interruptPending()) {
            uint16_t vec = port_->interruptVector();
            uint16_t sem = rd(vec);
            ctp_ = kNil;
            state_ = State::Running;
            t10_ += signalTime10(sem);  // timed as a SIGNAL (pmachineTiming.cpp)
            signal(sem);
        } else {
            state_ = State::Idle;
        }
        return;
    }
    ctp_ = rqp_;
    rqp_ = rd(ctp_);
    loadtib();
    state_ = State::Running;
}

void PMachineCore::signal(uint16_t sem) {
    // SIGNAL (2.21). The semaphore address is already off the stack (7.7). A semaphore is
    // two words: count, then the queue of waiting tasks.
    uint16_t count = rd(sem), waitq = rd((uint16_t)(sem + 1));
    if (count == 0 && waitq != kNil) {
        uint16_t task = waitq;  // deque the first waiter
        wr((uint16_t)(sem + 1), rd(task));
        rqp_ = enque(rqp_, task);
        if (ctp_ == kNil) { dispatch(); return; }
        if (prior(ctp_) < prior(task)) {  // the woken task runs first: pre-empt
            rqp_ = enque(rqp_, ctp_);
            taskswitch();
        }
        return;
    }
    wr(sem, (uint16_t)(count + 1));
    if (ctp_ == kNil) dispatch();
}

void PMachineCore::raise(uint16_t err) {
    // 2.6 Raise: push the error number and call segment 2, procedure 2, the system's
    // execution-error handler, as CXG 2,2 does.
    if (raising_) {
        stop("Raise " + std::to_string(err) + " while raising: no stack for the error handler");
        return;
    }
    raising_ = true;
    t10_ += kRaiseTime10;
    push(err);
    uint8_t caller = thisSegNo();
    if (call(getsegb(2), 2, bp_, caller)) addSegrefs(2, +1);  // as CXG counts it (2.19)
    raising_ = false;
}

bool PMachineCore::call(uint16_t newSegb, uint16_t proc, uint16_t statLink, uint8_t msseg) {
    // 7.2: procedure n's dictionary entry is the word at segb + segb^.i - n. It is a word
    // offset to the procedure's header word, which holds the data size; the code starts at
    // the next word.
    uint16_t entry = rd((uint16_t)(newSegb + rd(newSegb) - proc));
    uint16_t dataSz = rd((uint16_t)(newSegb + entry));

    // 2.18 createmscw prints: sp := sp - mscw_sz - data_sz, then stack overflow if
    // sp < splow, or if data_sz + mscw_sz > sp - splow. In 16 bits the subtraction can
    // wrap past 0 and pass the first test; the second catches that only with the OLD sp,
    // so the core reads it that way: the frame must fit ONCE (reference 7.12, settled on
    // the real machine in Stage 5). Read with the new sp, a call needs room twice over,
    // and the real Filer's L (a 260-word frame with 516 words free, after V) stops with
    // *STK OFLOW*, which the real machine does not do. In 32 bits the two tests are the
    // same test. The core changes sp only after it passes, so the error handler gets the
    // caller's stack, not a half-built frame.
    int32_t splow = rd((uint16_t)(ctp_ + kTibSplow));
    int32_t newSp = (int32_t)sp_ - kMscwSz - dataSz;
    if (newSp < splow) {
        raise(kStackOverflow);
        return false;
    }
    uint16_t lm = mp_;
    sp_ = (uint16_t)newSp;
    mp_ = sp_;
    wr((uint16_t)(mp_ + kMsDynl), lm);
    wr((uint16_t)(mp_ + kMsIpc), ipc_);
    wr((uint16_t)(mp_ + kMsStat), statLink);
    uint16_t w3 = rd((uint16_t)(mp_ + kMsSeg));
    wr((uint16_t)(mp_ + kMsSeg), (uint16_t)((w3 & 0xFF00) | msseg));  // msseg: low byte (7.11)
    segb_ = newSegb;
    ipc_ = (uint16_t)(2 * entry + 2);
    return true;
}

void PMachineCore::execute() {
    uint16_t at = ipc_;
    pcode::Decoded d = pcode::decode(at, [this](uint32_t a) { return codeByte(a); });
    if (!d.info) {
        // C-1 does not say what the MicroEngine does with an unassigned opcode (6.10).
        stop("unassigned opcode " + hex2(d.op) + " at SEGB " + hex4(segb_) + " IPC " + hex4(at));
        return;
    }
    t10_ = time10(d);               // from the operands, before the operator changes them
    ipc_ = (uint16_t)(at + d.len);  // jump offsets count from here (7.3)
    const int32_t* a = d.args;
    uint8_t op = d.op;

    // The short forms first: their value is in the opcode.
    if (op <= 0x1F) { push((uint16_t)a[0]); return; }                       // 2.1 SLDC
    if (op <= 0x2F) { push(rd(local((uint16_t)a[0]))); return; }            // 2.2 SLDL
    if (op <= 0x3F) { push(rd(global((uint16_t)a[0]))); return; }           // 2.3 SLDO
    if (op >= 0x78 && op <= 0x7F) {                                          // 2.9 SIND
        wr(sp_, rd((uint16_t)(tos() + a[0])));
        return;
    }

    switch (op) {
    // ---- 2.1 constant loads
    case 0x98: push(kNil); break;                                           // LDCN
    case 0x80: push((uint16_t)a[0]); break;                                 // LDCB
    case 0x81: push((uint16_t)a[0]); break;                                 // LDCI
    case 0x82: push((uint16_t)(segb_ + a[0])); break;                       // LCA

    // ---- 2.2 / 2.3 local and global
    case 0x87: push(rd(local((uint16_t)a[0]))); break;                      // LDL
    case 0x84: push(local((uint16_t)a[0])); break;                          // LLA
    case 0xA4: wr(local((uint16_t)a[0]), pop()); break;                     // STL
    case 0x85: push(rd(global((uint16_t)a[0]))); break;                     // LDO
    case 0x86: push(global((uint16_t)a[0])); break;                         // LAO
    case 0xA5: wr(global((uint16_t)a[0]), pop()); break;                    // SRO

    // ---- 2.4 intermediate: DB static links from mp
    case 0x89: push(rd((uint16_t)(walkStatic(mp_, a[0]) + 3 + a[1]))); break;  // LOD
    case 0x88: push((uint16_t)(walkStatic(mp_, a[0]) + 3 + a[1])); break;      // LDA
    case 0xA6: {                                                               // STR
        uint16_t lm = walkStatic(mp_, a[0]);
        wr((uint16_t)(lm + 3 + a[1]), pop());
        break;
    }

    // ---- 2.5 indirect store
    case 0xC4: {                                                            // STO
        uint16_t v = pop();
        uint16_t p = pop();
        wr(p, v);
        break;
    }

    // ---- 2.6 extended (through getsegb, the reading of the 2.6 note)
    case 0x9A: push(rd((uint16_t)(getsegb((uint16_t)a[0]) + a[1]))); break;  // LDE
    case 0x9B: push((uint16_t)(getsegb((uint16_t)a[0]) + a[1])); break;      // LAE
    case 0xD9: wr((uint16_t)(getsegb((uint16_t)a[0]) + a[1]), pop()); break; // STE

    // ---- 2.7 multiple words
    case 0x83: {                                                            // LDC
        uint16_t src = (uint16_t)(segb_ + a[0] + a[1]);
        for (int i = 1; i <= a[1]; ++i) wr((uint16_t)(sp_ - i), rd((uint16_t)(src - i)));
        sp_ = (uint16_t)(sp_ - a[1]);
        break;
    }
    case 0xD0: {                                                            // LDM
        uint16_t src = (uint16_t)(tos() + a[0]);
        ++sp_;
        for (int i = 1; i <= a[0]; ++i) wr((uint16_t)(sp_ - i), rd((uint16_t)(src - i)));
        sp_ = (uint16_t)(sp_ - a[0]);
        break;
    }
    case 0x8E: {                                                            // STM (7.5)
        uint16_t dst = rd((uint16_t)(sp_ + a[0]));
        for (int i = 0; i < a[0]; ++i) wr((uint16_t)(dst + i), rd((uint16_t)(sp_ + i)));
        sp_ = (uint16_t)(sp_ + a[0] + 1);
        break;
    }

    // ---- 2.8 bytes: a byte pointer is two words, the word address under the index
    case 0xA7: {                                                            // LDB
        int16_t idx = (int16_t)pop();
        uint16_t base = pop();
        push(port_->readByte(((uint32_t)base * 2 + idx) & 0x1FFFF));
        break;
    }
    case 0xC8: {                                                            // STB
        uint8_t v = (uint8_t)pop();
        int16_t idx = (int16_t)pop();
        uint16_t base = pop();
        port_->writeByte(((uint32_t)base * 2 + idx) & 0x1FFFF, v);
        break;
    }

    // ---- 2.9 records and arrays
    case 0xC5: {                                                            // MOV
        uint16_t src = pop();
        uint16_t dst = pop();
        for (int i = 0; i < a[0]; ++i) wr((uint16_t)(dst + i), rd((uint16_t)(src + i)));
        break;
    }
    case 0xE6: wr(sp_, rd((uint16_t)(tos() + a[0]))); break;                // IND
    case 0xE7: wr(sp_, (uint16_t)(tos() + a[0])); break;                    // INC
    case 0xD7: {                                                            // IXA
        int16_t i = (int16_t)pop();
        wr(sp_, (uint16_t)(tos() + i * a[0]));
        break;
    }
    case 0xD8: {                                                            // IXP
        if (a[0] == 0) { stopAt(at, "IXP with 0 elements per word"); return; }
        int16_t inx = (int16_t)tos();
        wr((uint16_t)(sp_ + 1), (uint16_t)(rd((uint16_t)(sp_ + 1)) + inx / a[0]));
        wr(sp_, (uint16_t)a[1]);
        push((uint16_t)((inx % a[0]) * a[1]));
        break;
    }
    case 0xC9: {                                                            // LDP (2.9 note)
        unsigned start = pop() & 0xF;
        unsigned width = pop();
        uint16_t w = rd(tos());
        uint32_t mask = width >= 16 ? 0xFFFFu : ((1u << width) - 1);
        wr(sp_, (uint16_t)((w >> start) & mask));
        break;
    }
    case 0xCA: {                                                            // STP
        uint16_t v = pop();
        unsigned start = pop() & 0xF;
        unsigned width = pop();
        uint16_t p = pop();
        uint32_t mask = (width >= 16 ? 0xFFFFu : ((1u << width) - 1)) << start;
        uint16_t w = rd(p);
        wr(p, (uint16_t)((w & ~mask) | (((uint32_t)v << start) & mask)));
        break;
    }

    // ---- 2.10 logicals
    case 0xA1: { uint16_t y = pop(); wr(sp_, tos() & y); break; }           // LAND
    case 0xA0: { uint16_t y = pop(); wr(sp_, tos() | y); break; }           // LOR
    case 0xE5: wr(sp_, (uint16_t)~tos()); break;                            // LNOT
    case 0x9F: wr(sp_, (uint16_t)(~tos() & 1)); break;                      // BNOT (7.11)
    case 0xB4: { uint16_t y = pop(); wr(sp_, b(tos() <= y)); break; }       // LEUSW
    case 0xB5: { uint16_t y = pop(); wr(sp_, b(tos() >= y)); break; }       // GEUSW

    // ---- 2.11 integers: 16-bit two's complement, wrapping
    case 0xE0: { int16_t v = (int16_t)tos(); wr(sp_, (uint16_t)(v < 0 ? -v : v)); break; }  // ABI
    case 0xE1: wr(sp_, (uint16_t)(-(int32_t)(int16_t)tos())); break;        // NGI
    case 0xE2: push(tos()); break;                                          // DUP1
    case 0xA2: { uint16_t y = pop(); wr(sp_, (uint16_t)(tos() + y)); break; }  // ADI
    case 0xA3: { uint16_t y = pop(); wr(sp_, (uint16_t)(tos() - y)); break; }  // SBI
    case 0x8C: {                                                            // MPI
        int32_t y = (int16_t)pop();
        wr(sp_, (uint16_t)((int32_t)(int16_t)tos() * y));
        break;
    }
    case 0x8D: {                                                            // DVI
        int32_t y = (int16_t)tos();
        if (y == 0) { raise(kDivByZero); break; }
        ++sp_;
        wr(sp_, (uint16_t)((int32_t)(int16_t)tos() / y));  // Pascal div truncates, as C++
        break;
    }
    case 0x8F: {                                                            // MODI
        int32_t y = (int16_t)tos();
        if (y <= 0) { raise(kModByNonPos); break; }
        ++sp_;
        int32_t x = (int16_t)tos();
        // The printed comment "-x mod x returns x not 0" (2.11) describes a microcode that,
        // for a negative x, answers y - (|x| mod y). That gives the Pascal (non-negative)
        // result for every x, except a multiple of y, which answers y instead of 0.
        int32_t m = x >= 0 ? x % y : y - ((-x) % y);
        wr(sp_, (uint16_t)m);
        break;
    }
    case 0xCB: {                                                            // CHK
        int16_t hi = (int16_t)rd(sp_), lo = (int16_t)rd((uint16_t)(sp_ + 1));
        int16_t v = (int16_t)rd((uint16_t)(sp_ + 2));
        if (lo <= v && v <= hi) sp_ = (uint16_t)(sp_ + 2);
        else raise(kRangeError);
        break;
    }
    case 0xB0: { int16_t y = (int16_t)pop(); wr(sp_, b((int16_t)tos() == y)); break; }  // EQUI
    case 0xB1: { int16_t y = (int16_t)pop(); wr(sp_, b((int16_t)tos() != y)); break; }  // NEQI
    case 0xB2: { int16_t y = (int16_t)pop(); wr(sp_, b((int16_t)tos() <= y)); break; }  // LEQI
    case 0xB3: { int16_t y = (int16_t)pop(); wr(sp_, b((int16_t)tos() >= y)); break; }  // GEQI

    // ---- 2.12, 2.13 reals: two words, the high word on top (7.4)
    case 0xCC: {                                                            // FLT
        float f = (float)(int16_t)tos();
        --sp_;
        putReal(sp_, f);
        break;
    }
    case 0xBE: case 0xBF: {                                                 // TNC, RND
        float f = getReal(sp_);
        // RND is Pascal's round: half away from zero.
        float t = op == 0xBE ? std::trunc(f) : std::round(f);
        if (!(t >= -32768.0f && t <= 32767.0f)) { raise(kFloatError); break; }  // C-1
        ++sp_;
        wr(sp_, (uint16_t)(int16_t)t);
        break;
    }
    case 0xE3: wr(sp_, (uint16_t)(tos() & 0x7FFF)); break;                  // ABR: clear the sign
    case 0xE4: wr(sp_, (uint16_t)(tos() ^ 0x8000)); break;                  // NGR: flip the sign
    case 0xC6:                                                              // DUP2
        sp_ = (uint16_t)(sp_ - 2);
        wr(sp_, rd((uint16_t)(sp_ + 2)));
        wr((uint16_t)(sp_ + 1), rd((uint16_t)(sp_ + 3)));
        break;
    case 0xC0: case 0xC1: case 0xC2: case 0xC3: {                           // ADR SBR MPR DVR
        float y = getReal(sp_), x = getReal((uint16_t)(sp_ + 2));
        float r;
        switch (op) {
        case 0xC0: r = x + y; break;
        case 0xC1: r = x - y; break;
        case 0xC2: r = x * y; break;
        default:
            if (y == 0) { raise(kFloatError); return; }
            r = x / y;
            break;
        }
        if (!realResult(r)) break;
        sp_ = (uint16_t)(sp_ + 2);
        putReal(sp_, r);
        break;
    }
    case 0xCD: case 0xCE: case 0xCF: {                                      // EQU LEQ GEQREAL
        float y = getReal(sp_), x = getReal((uint16_t)(sp_ + 2));
        bool v = op == 0xCD ? x == y : op == 0xCE ? x <= y : x >= y;  // GEQREAL is >= (2.13)
        sp_ = (uint16_t)(sp_ + 3);
        wr(sp_, b(v));
        break;
    }

    // ---- 2.14, 2.15 sets; 2.16 byte-array compares
    case 0xC7: case 0xBC: case 0xDA: case 0xDB: case 0xDC: case 0xDD:
    case 0xB6: case 0xB7: case 0xB8:
        execSet(op, d.nargs ? a[0] : 0);
        break;
    case 0xB9: case 0xBA: case 0xBB: execByteCompare(op, a[0]); break;

    // ---- 2.18, 2.19 calls and return; 2.20 static link
    case 0x90: call(segb_, (uint16_t)a[0], mp_, 0); break;                  // CPL
    case 0x91: call(segb_, (uint16_t)a[0], bp_, 0); break;                  // CPG
    case 0x92: call(segb_, (uint16_t)a[1], walkStatic(mp_, a[0]), 0); break; // CPI (7.6)
    case 0x93: callExternal((uint16_t)a[0], (uint16_t)a[1], mp_); break;    // CXL
    case 0x94: callExternal((uint16_t)a[0], (uint16_t)a[1], bp_); break;    // CXG
    case 0x95:                                                              // CXI (7.6)
        callExternal((uint16_t)a[0], (uint16_t)a[2], walkStatic(mp_, a[1]));
        break;
    case 0x97: {                                                            // CPF
        // A formal procedure: on top, segment number (high byte) and procedure number (low
        // byte); under it, the static link. msseg gets the caller's segment, as CXL (2.19).
        uint16_t ls = pop();
        uint16_t lm = pop();
        callExternal((uint16_t)(ls >> 8), (uint16_t)(ls & 0xFF), lm);
        break;
    }
    case 0x96: {                                                            // RPU
        uint16_t lm = mp_;
        sp_ = mp_;
        mp_ = rd((uint16_t)(lm + kMsDynl));
        ipc_ = rd((uint16_t)(lm + kMsIpc));
        uint8_t msseg = (uint8_t)rd((uint16_t)(lm + kMsSeg));
        if (msseg != 0) {
            addSegrefs(thisSegNo(), -1);  // the segment being left (2.19 note)
            segb_ = getsegb(msseg);
        }
        sp_ = (uint16_t)(sp_ + a[0] + kMscwSz);
        break;
    }
    case 0x99: push(walkStatic(mp_, a[0])); break;                          // LSL

    // ---- 2.21 concurrency
    case 0xDE: signal(pop()); break;                                        // SIGNAL
    case 0xDF: {                                                            // WAIT
        uint16_t sem = pop();  // popped in every case (7.7)
        uint16_t count = rd(sem);
        if (count == 0) {
            wr((uint16_t)(sem + 1), enque(rd((uint16_t)(sem + 1)), ctp_));
            taskswitch();
        } else {
            wr(sem, (uint16_t)(count - 1));  // a wait decrements (2.21 note)
        }
        break;
    }

    // ---- 2.22 processor registers, breakpoint
    case 0x9D: {                                                            // LPR
        int16_t r = (int16_t)tos();
        if (r < -3) { stopAt(at, "LPR of register " + std::to_string(r)); return; }
        if (r >= 0) updatetib();
        uint16_t v = r == -3 ? rqp_ : r == -2 ? sdp_ : r == -1 ? ctp_ : rd((uint16_t)(ctp_ + r));
        wr(sp_, v);
        break;
    }
    case 0xD1: {                                                            // SPR (7.8)
        int16_t r = (int16_t)rd((uint16_t)(sp_ + 1));
        if (r < -3) { stopAt(at, "SPR of register " + std::to_string(r)); return; }
        uint16_t v = pop();
        pop();
        if (r == -3) { rqp_ = v; break; }
        if (r == -2) { sdp_ = v; break; }
        updatetib();
        if (r == -1) {
            ctp_ = v;  // a new current task: run it from its TIB (7.11)
        } else {
            wr((uint16_t)(ctp_ + r), v);
        }
        loadtib();
        break;
    }
    case 0x9E: raise(kBreakpoint); break;                                   // BPT

    // ---- 2.17 jumps, from the next instruction (7.3); a BOOLEAN is its bit 0
    case 0x8A: case 0x8B: ipc_ = (uint16_t)(ipc_ + a[0]); break;            // UJP, UJPL
    case 0xD4: case 0xD5:                                                   // FJP, FJPL
        if (!(pop() & 1)) ipc_ = (uint16_t)(ipc_ + a[0]);
        break;
    case 0xD2: { uint16_t y = pop(); if (pop() != y) ipc_ = (uint16_t)(ipc_ + a[0]); break; }  // EFJ
    case 0xD3: { uint16_t y = pop(); if (pop() == y) ipc_ = (uint16_t)(ipc_ + a[0]); break; }  // NFJ
    case 0xD6: {                                                            // XJP
        int16_t v = (int16_t)pop();
        uint16_t tbl = (uint16_t)(segb_ + a[0]);
        int16_t lo = (int16_t)rd(tbl), hi = (int16_t)rd((uint16_t)(tbl + 1));
        if (lo <= v && v <= hi) ipc_ = (uint16_t)(ipc_ + rd((uint16_t)(tbl + 2 + v - lo)));
        break;
    }

    // ---- 2.22 miscellaneous
    case 0x9C: break;                                                       // NOP
    case 0xBD: {                                                            // SWAP
        uint16_t x = rd(sp_);
        wr(sp_, rd((uint16_t)(sp_ + 1)));
        wr((uint16_t)(sp_ + 1), x);
        break;
    }

    default:
        stopAt(at, std::string("internal: no case for operator ") + d.info->mnem);  // a slip
        break;
    }
}

// A set on the stack (2.14): its length in words on top, the words above it, word 0 at
// sp + 1. "len0" is the set on top (the right operand), "len1" the one under it (the left).
// Where C-3 writes `(sp + k)^.p` for a set operand it means the ADDRESS sp + k (2.14 note).
void PMachineCore::execSet(uint8_t op, int32_t ub) {
    auto W = [this](int32_t off) { return (uint16_t)(sp_ + off); };
    switch (op) {
    case 0xC7: {                                                            // ADJ UB
        int32_t len = tos();
        uint16_t src = W(1), dst = W(len - ub + 1);
        if (len > ub) {         // shrink: move the low UB words up, top first
            for (int32_t i = 1; i <= ub; ++i)
                wr((uint16_t)(dst + ub - i), rd((uint16_t)(src + ub - i)));
        } else if (len < ub) {  // expand: move down, bottom first, then zero the rest
            for (int32_t i = 0; i < len; ++i) wr((uint16_t)(dst + i), rd((uint16_t)(src + i)));
            for (int32_t i = len; i < ub; ++i) wr((uint16_t)(dst + i), 0);
        }
        sp_ = W(len - ub + 1);  // the length word is gone (C-1)
        break;
    }
    case 0xBC: {                                                            // SRS
        int32_t hi = (int16_t)tos(), lo = (int16_t)rd(W(1));
        constexpr int32_t bset = 4080;
        if (!(0 <= hi && hi <= bset - 1 && 0 <= lo && lo <= bset - 1)) {
            raise(kRangeError);
            break;
        }
        if (lo > hi) {
            sp_ = W(1);
            wr(sp_, 0);  // the null set
            break;
        }
        int32_t len = hi / 16 + 1;
        sp_ = W(-len + 1);
        wr(sp_, (uint16_t)len);
        for (int32_t wi = 0; wi < len; ++wi) {
            uint16_t word = 0;
            for (int bit = 0; bit < 16; ++bit) {
                int32_t n = wi * 16 + bit;
                if (lo <= n && n <= hi) word |= (uint16_t)(1u << bit);
            }
            wr(W(1 + wi), word);
        }
        break;
    }
    case 0xDA: {                                                            // INN
        int32_t len = tos();
        int32_t val = (int16_t)rd(W(len + 1));
        bool in = false;
        if (0 <= val && val <= len * 16 - 1) in = (rd(W(1 + val / 16)) >> (val % 16)) & 1;
        sp_ = W(len + 1);
        wr(sp_, b(in));
        break;
    }
    case 0xDB: {                                                            // UNI
        int32_t len0 = tos(), len1 = rd(W(len0 + 1));
        if (len1 >= len0) {
            uint16_t src = W(1), dst = W(len0 + 2);
            for (int32_t i = 0; i < len0; ++i)
                wr((uint16_t)(dst + i), rd((uint16_t)(dst + i)) | rd((uint16_t)(src + i)));
            sp_ = W(len0 + 1);
        } else {
            uint16_t src = W(len0 + 2), dst = W(1);
            for (int32_t i = 0; i < len1; ++i)
                wr((uint16_t)(dst + i), rd((uint16_t)(dst + i)) | rd((uint16_t)(src + i)));
            // Move the longer set up over the shorter one, its length word with it.
            uint16_t from = W(len0), to = W(len0 + len1 + 1);  // C-3 "lp + len0": sp (2.14)
            for (int32_t i = 0; i <= len0; ++i)
                wr((uint16_t)(to - i), rd((uint16_t)(from - i)));
            sp_ = W(len1 + 1);
        }
        break;
    }
    case 0xDC: {                                                            // INT
        int32_t len0 = tos(), len1 = rd(W(len0 + 1));
        uint16_t src = W(1), dst = W(len0 + 2);
        if (len0 == 0) {
            sp_ = W(len1 + 1);
            wr(sp_, 0);
        } else if (len1 == 0) {
            sp_ = W(len0 + 1);
        } else if (len1 > len0) {
            for (int32_t i = 0; i < len0; ++i)
                wr((uint16_t)(dst + i), rd((uint16_t)(dst + i)) & rd((uint16_t)(src + i)));
            for (int32_t i = len0; i < len1; ++i) wr((uint16_t)(dst + i), 0);
            sp_ = W(len0 + 1);
        } else {
            for (int32_t i = 0; i < len1; ++i)
                wr((uint16_t)(dst + i), rd((uint16_t)(dst + i)) & rd((uint16_t)(src + i)));
            sp_ = W(len0 + 1);
        }
        break;
    }
    case 0xDD: {                                                            // DIF
        int32_t len0 = tos(), len1 = rd(W(len0 + 1));
        uint16_t src = W(1), dst = W(len0 + 2);
        if (len0 == 0) { sp_ = W(1); break; }
        if (len1 == 0) { sp_ = W(len0 + 1); break; }
        int32_t n = len1 > len0 ? len0 : len1;
        for (int32_t i = 0; i < n; ++i)
            wr((uint16_t)(dst + i), rd((uint16_t)(dst + i)) & (uint16_t)~rd((uint16_t)(src + i)));
        sp_ = W(len0 + 1);
        break;
    }
    case 0xB6: case 0xB7: case 0xB8: {                                      // EQU LEQ GEQPWR
        int32_t len0 = tos(), len1 = rd(W(len0 + 1));
        bool v = scanPwr(op).v;
        sp_ = W(len0 + len1 + 1);
        wr(sp_, b(v));
        break;
    }
    }
}

// The two sets as in execSet: len0 (right operand) on top, len1 (left) under it. n counts
// the common words compared, the deciding word included; d counts the extra words of the
// longer set examined (M0015's N and D).
PMachineCore::PwrScan PMachineCore::scanPwr(uint8_t op) {
    auto W = [this](int32_t off) { return (uint16_t)(sp_ + off); };
    int32_t len0 = tos(), len1 = rd(W(len0 + 1));
    uint16_t src = W(1), dst = W(len0 + 2);  // src: right operand, dst: left operand
    int32_t minl = len0 < len1 ? len0 : len1, maxl = len0 < len1 ? len1 : len0;
    auto s = [&](int32_t i) { return rd((uint16_t)(src + i)); };
    auto l = [&](int32_t i) { return rd((uint16_t)(dst + i)); };
    auto pass = [&](int32_t i) {
        if (op == 0xB6) return s(i) == l(i);
        if (op == 0xB7) return s(i) == (uint16_t)(l(i) | s(i));  // left within right
        return l(i) == (uint16_t)(l(i) | s(i));                    // right within left
    };
    PwrScan r{false, 0, 0};
    int32_t i = 0;
    while (i < minl && pass(i)) ++i;
    r.n = i < minl ? i + 1 : minl;
    if (i < minl) return r;  // a common word decides: the extra words are not examined
    // The extra words must be 0. EQUPWR checks the longer set's; LEQPWR only the left
    // set's (a subset); GEQPWR only the right set's (a superset): 2.15 note.
    bool rightLonger = len0 > len1, leftLonger = len1 > len0;
    bool check = op == 0xB6 ? (rightLonger || leftLonger)
               : op == 0xB7 ? leftLonger : rightLonger;
    int32_t k = minl;
    if (check)
        while (k < maxl && (rightLonger ? s(k) : l(k)) == 0) ++k;
    r.d = !check ? 0 : k < maxl ? k - minl + 1 : maxl - minl;
    r.v = !check || k >= maxl;
    return r;
}

// 2.16: two word pointers to byte arrays of `len` bytes; TOS-1 is the left operand (dst),
// TOS the right (src). Compared byte by byte from byte 0, the first difference deciding
// (lexicographic, the 2.16 note), each byte unsigned.
PMachineCore::ByteScan PMachineCore::scanBytes(int32_t len) {
    uint16_t src = tos(), dst = rd((uint16_t)(sp_ + 1));
    ByteScan r{0, 0};
    while (r.n < len && r.cmp == 0) {
        uint8_t l = port_->readByte(((uint32_t)dst * 2 + r.n) & 0x1FFFF);
        uint8_t x = port_->readByte(((uint32_t)src * 2 + r.n) & 0x1FFFF);
        r.cmp = (l > x) - (l < x);
        ++r.n;  // bytes compared, the deciding byte included
    }
    return r;
}

void PMachineCore::execByteCompare(uint8_t op, int32_t len) {
    int cmp = scanBytes(len).cmp;
    ++sp_;
    bool v = op == 0xB9 ? cmp == 0 : op == 0xBA ? cmp <= 0 : cmp >= 0;
    wr(sp_, b(v));
}

void PMachineCore::serialize(StateWriter& w) const {
    w.u16(sp_); w.u16(mp_); w.u16(bp_); w.u16(ipc_);
    w.u16(segb_); w.u16(ctp_); w.u16(rqp_); w.u16(sdp_);
    w.u8((uint8_t)state_);
    w.str(stopReason_);
    w.u8(carry10_);
}

void PMachineCore::deserialize(StateReader& r) {
    sp_ = r.u16(); mp_ = r.u16(); bp_ = r.u16(); ipc_ = r.u16();
    segb_ = r.u16(); ctp_ = r.u16(); rqp_ = r.u16(); sdp_ = r.u16();
    uint8_t s = r.u8();
    state_ = s <= (uint8_t)State::Stopped ? (State)s : State::Stopped;
    stopReason_ = r.str();
    carry10_ = (uint8_t)(r.u8() & 3);
    raising_ = false;
}

} // namespace altair
