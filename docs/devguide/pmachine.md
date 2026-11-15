# The P-machine core: the Pascal MicroEngine as a chip

> **Status:** the core runs every WD III.0 operator and has tests. The `pascal100` board
> (docs/boards/digicomp-pascal100.md) uses it, and the Pascal-100 machine (`examples/pascal`) boots a real III.0
> system disk with it.

`PMachineCore` is the Western Digital Pascal MicroEngine as a chip. It runs WD III.0 P-code
the way WD's own documents define it. It is a `CpuCore` (DESIGN.md §3), with one difference:
it reads memory through a `WordPort`, not through the `Bus`.

## Files

| File | What it holds |
|---|---|
| `src/cpu/pcodeOps.{h,cpp}` | The one operator table: mnemonic, opcodes, parameter kinds. `decode()` and `encode()` use it. |
| `src/cpu/cpuPmachine.{h,cpp}` | The core: registers, boot, the operators, tasks, `Raise`. |
| `src/cpu/pmachineTiming.cpp` | The time of each operator, from WD Technical Note M0015. |
| `src/isa/isaPcode.cpp` | The `pcode` disassembler (`DISASM CPU=pcode`). |
| `tests/pcode_fixture.h` | The test encoder `pfix::pasm()` and the mini-system `pfix::Sys`. |
| `tests/test_pmachine.cpp` | The tests (`./build/altair_tests pmachine`). |

There is no P-code assembler, by design. The real system had none: everything was written in
Pascal, and the compiler made the P-code. The test encoder exists only to write test programs.

## Where the facts come from

All sources are period documents. No fact comes from another emulator.

- **Table C-3 and C-1** in
  `reference/80-0130016-00A0_UCSD_Pascal_III.0_Operating_System_Reference_Manual_Jul82.md`.
  Section 2 of that file is the operator semantics. Section 7 holds the readings that real
  code settles where the printed table is unclear. The core's `// 7.x` comments point there.
- **The Pascal-100 manual**, Appendix F (bootstrap flowchart, TIB) and Appendix H (run-time
  errors, the opcode list, and M0015), in `Reference/pascal100/`.
- **The owner's distribution disk 033.** Its code files settled the procedure dictionary,
  the jump base, the real word order and other points (reference §7).

The open questions that only the real machine can answer are in reference §7.11. That list
is the one place they are kept. When the real machine settles one, it moves to §7.12.

## The memory port

The MicroEngine is word-addressed: 64K words of 16 bits. A byte address is 17 bits, and byte
`2w` is the low half of word `w`. On the Pascal-100 the chip reaches memory through the
board's Map and its 16-bit transfer logic. Those belong to the board, so the board attaches a
`WordPort` to the core:

- `readWord` and `writeWord` are the memory cycles.
- `readByte` and `writeByte` default to a read-modify-write of the word.
- `interruptPending` and `interruptVector` are the interrupt line. The call to
  `interruptVector()` is the acknowledge.

`step(Bus&)` does not use the `Bus` it gets. The tests attach a plain 64K-word memory.

## Run states

| State | Meaning | `halted()` |
|---|---|---|
| Start | After `reset()`. The next step runs the boot from word `FC68`. | no |
| Running | Executes one operator for each step. | no |
| Idle | Table C-3 label 6: no task is ready. Only an interrupt ends it. | yes |
| Stopped | The core stopped and `stopReason()` says why. | yes |

The core stops when it meets something that the documents do not define, for example an
unassigned opcode. It does not guess.

## Timing

`step()` returns the M0015 time of the operator as cycles of the 2.5 MHz clock (one cycle is
0.4 µs). The core does the arithmetic in tenths of a microsecond. A few printed times are not
whole cycles (6.6, 16.6, 26.6, 11.4, 21.8, 159.4 and 140.6 µs). The odd tenths carry to the
next step, so the total time stays exact.

`time10()` reads the operands before the operator runs. Where M0015 prints a formula, the core
uses it. Where M0015 prints only a range, the core uses the printed "typical" time if there is
one. If not, it uses the middle of the range:

| Operator | Case | Time used (µs) | Printed |
|---|---|---|---|
| IXP | between the best and the worst index | middle of the two | best and worst only |
| IXP | an elements-per-word value with no row | 33.6 | rows for 3, 4, 5, 8, 16 |
| MPI | no operand is 0 | 28.0 | 5.2 to 35.2, "typical 28.0" |
| ADR | TOS-1 not 0 | 106.8 | 60.8 to 152.8 |
| SBR | TOS-1 not 0 | 108.2 | 64.4 to 152.0 |
| MPR | TOS-1 not 0 | 168.6 | 159.4 to 177.8 |
| DVR | TOS not 0 | 218.2 | 140.6 to 295.8 |

IXA has no formula, so its times are measured. M0015 prints 9.6 at index 0 and a range to 56.8,
and says that the time depends on the element size. These are from a Pascal-100 at 3.0 MHz, scaled
to 2.5 MHz, in µs:

| Element size | 1 | 2 | 3 | 4 | 8 | 16 | 100 |
|---|---|---|---|---|---|---|---|
| Index 1 | 18.7 | 19.0 | 19.3 | 19.1 | 19.1 | 20.9 | 21.3 |
| Index 7 | 19.9 | 22.1 | 23.9 | 24.6 | 27.3 | 29.1 | 29.1 |

A size between two columns takes the lower column. An index of other lengths is an estimate:
each bit adds the same time, up to 56.8. Each value is one run, good to about 0.3 µs.

The other operators use M0015 as printed, and the real machine differs from it. Measured on the
same box, in the same units:

| Statement | Operators | M0015 | Box |
|---|---|---|---|
| `K:=J` | SLDL STL | 16.0 | 15.6 |
| `K:=J+1` | SLDL SLDC ADI STL | 26.8 | 26.4 |
| `F:=J<=K` | SLDL SLDL LEQI STL | 32.8 | 32.2 |
| `IF Z=0`, false | SLDL SLDC EQUI FJP, jump | 36.4 | 34.5 |
| `IF Z=1`, true, `K:=J` | SLDL SLDC EQUI FJP, SLDL STL | 45.6 | 42.0 |
| A call with no parameters | CPG RPU | 70.4 | 62.4 |
| `FOR` loop control | SLDL LDL LEQI FJP SLDL SLDC ADI STL UJP | 76.4 | 69.5 |

So calls and loops run 9 to 11% faster on the box than the core. The core keeps the printed values:
CPG and RPU always run as a pair, so a split between them would be a guess, and the loop result
does not show which operator differs. The cause is not known.

These readings of the M0015 rows are the core's choice. The real machine can test them:

- SRS: the first time, 18.0, has no label. The core uses it for the null set and for a range error.
- ADJ: compression uses the formula as printed, `16.4 + 5.6(words(TOS)) + 2.8(UB - words(TOS))`.
  The last term is negative for a compression.
- LEQREAL and GEQREAL: the core picks the row by the result (equal, TRUE, FALSE), not by the
  operand order in the row labels. "pos < pos" is read as "both operands are positive".
- DVI: "negative result" means the operand signs differ. MODI: it means a negative dividend.
- A Raise adds 63.2 µs: M0015 gives BPT "comparable to the time for CXG". An operator that
  raises with no printed time for that case takes its shortest printed time first.
- An interrupt at label 6 is timed as a SIGNAL: 134.8 µs if a task wakes, else the SIGNAL
  time for its count.
- The boot, an idle step and a stop have no printed time. Each counts one cycle, so a clock
  fed by these cycles still runs.

## Tests

`pfix::Sys` builds a small P-machine system in plain memory: a TIB, the segment vector and
SIBs, segment 2 as the error handler, and segment 1 for the program under test. A segment uses
the layout of real code files (reference §7.2). The memory map is in the comment of
`tests/pcode_fixture.h`.

```cpp
Sys s;
s.prog("SLDC 3; SLDC 4; ADI");  // procedure 1 of segment 1, booted through FC68
CHECK(s.run() && s.tos() == 7, "ADI");
```

Two rules hold for the whole test suite:

- **The coverage gate.** `test_core_coverage()` runs last. It fails if any of the 168
  assigned opcodes did not run in some test, or if an operator that ran has no M0015 time.
- **Break the fix.** A test of a settled reading must fail when the code goes back to the
  printed reading. Do this by hand when you change a reading.

The test code does no I/O. On the real system, all I/O goes through the Z80 BIOS by the CPU
switch, so a test program that does I/O cannot run on the core alone. The real machine can
compile a test program when a test needs real compiler output.

## Not built yet

- `DISASM <addr> CPU=pcode` takes a byte address in memory, not an IPC: it does not follow
  SEGB. REGS, STEP and HISTORY CPU do follow SEGB.
