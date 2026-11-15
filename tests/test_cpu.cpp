#include "test.h"

#include "core/machine.h"

using namespace altair;

// pascal100 2026-11-09: the 8080 board and core are not in this build, and with them the
// sections that tested the 8080 core itself (flags, T-states, EI/HLT, RST 7 from an empty
// socket). The 8085 and Z80 cores keep their exercisers (tests/cputest.cpp).
void test_cpu() {
    SECTION("every core -- captureRegs() IS registers(), value for value, in order");

    // captureRegs() is the recorder's fast copy of the register list, and a second copy
    // of its ORDER. Hold every core's to its own registers(): scramble the whole machine
    // through the reflected setters, then the two must agree on every entry. Many rounds,
    // so every flag and every half is seen both ways.
    for (const char* type : {"8085", "z80"}) {
        Machine cm;
        std::string cerr;
        cm.add(type, "cpu0", cerr);
        CpuCore* core = cm.cpu();
        CHECK(core != nullptr, "the core under test is in the machine");
        if (!core) continue;

        std::vector<RegDef> defs = core->registers();
        std::vector<uint32_t> fast;
        uint32_t seed = 0x2468ACE1u;
        bool same = true;
        for (int round = 0; round < 64 && same; ++round) {
            for (const RegDef& d : defs) {
                seed = seed * 1664525u + 1013904223u;  // any spread will do
                d.set(seed >> 8);
            }
            core->captureRegs(fast);
            same = fast.size() == defs.size();
            for (size_t i = 0; same && i < defs.size(); ++i) same = fast[i] == defs[i].get();
        }
        std::string what = std::string(type) + ": captureRegs() matches registers() exactly";
        CHECK(same, what.c_str());
    }
}
