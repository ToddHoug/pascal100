# 8088 hardware test sample

`sample.txt` holds 12 tests from each of the 323 opcode files of the **SingleStepTests 8088 v2**
suite by Daniel Balsom (MIT licence, https://github.com/SingleStepTests/8088, version 2.0.1),
recorded from a real NMOS 8088 through an Arduino8088 interface. The full suite (3,007,000 tests,
about 730 MB) is not in the repository; the run against all of it is described in
`docs/sources.md` (the x86 exception) and in the project folder.

The 12 are spread evenly through each file. `tests/test_cpu8088.cpp` runs them through
`Cpu8088` on a real `Bus` with a 1 MB `memory` board.

## Format (one line each, per test)

| Line | Content |
|---|---|
| `FILE op mask status n` | starts an opcode file: name (`80.7` = opcode 80h, ModRM reg 7), the flags mask for undefined flags, the suite's status (`normal`, `undocumented`, `alias`, `fpu`, ...), the number of tests |
| `N text` | the test's name (its disassembly) |
| `B xx ..` | the instruction bytes, prefixes included |
| `I ...` | initial AX BX CX DX CS SS DS ES SP BP SI DI IP FLAGS (decimal) |
| `R n a v ..` | initial RAM: n (address, byte) pairs |
| `F ...` | final registers, same order; `-1` = unchanged |
| `W n a v ..` | final RAM that changed |
| `Q q c` | the prefetch queue length at the start (0 = empty), and the real cycle count |
