# The Host Bridge utilities

`R.COM`, `W.COM` and `HDIR.COM` — how a file gets into and out of a running CP/M guest. For
the CPU 8085/88 (CP/M-86, CP/M 8-16 and MP/M 8-16), `R.CMD`, `W.CMD` and `HDIR.CMD` do the
same (see *The 8086 versions* below).

```
HDIR [pattern]                  what is on the host, ls -l style: size, date, name
R  <hostfile> [cpmfile]         host -> CP/M      R *.ASM     R SRC/FOO.ASM
W  <cpmfile> [hostfile] [B|T]   CP/M -> host      W *.HEX     W FOO.DAT T
W  <pattern> [hostfolder] [B|T]                   W *.ASM SRC
```

They talk to the `hostbridge` card (`docs/boards/hostbridge.md`), which is in the default
machine at port **0xB0** and is rooted at a sandboxed `hostdir` — by default, the directory
you ran `pascal100` from.

## What is in here

| | |
|---|---|
| `R.ASM` `W.ASM` `HDIR.ASM` | the sources. 8080, `ORG 100H`, BDOS only. |
| `HB.INC` | the master equates — ports, commands, status bits, error codes |
| `R.COM` `W.COM` `HDIR.COM` | **the programs, ready to run** |
| `R.HEX` `W.HEX` `HDIR.HEX` | what `ASM` produced, and what `LOAD` turned into the `.COM` |
| `R.PRN` `W.PRN` `HDIR.PRN` | the assembler listings |
| `R.A86` `W.A86` `HDIR.A86` | the 8086 sources, for CP/M-86, CP/M 8-16 and MP/M 8-16. BDOS only (`INT 224`). |
| `R.CMD` `W.CMD` `HDIR.CMD` | **the 8086 programs, ready to run** |
| `R.H86` `W.H86` `HDIR.H86` | what `ASM86` produced, and what `GENCMD` turned into the `.CMD` |
| `R.LST` `W.LST` `HDIR.LST` | the 8086 assembler listings |

**The `.COM`, `.HEX` and `.PRN`, and the 8086 `.CMD`, `.H86` and `.LST`, are the only build
artifacts committed anywhere in this repository**, and they are here for one reason: so that somebody who clones it has working
utilities without first pasting 28 KB of assembler into `PIP`. They are not built by CMake
and no host toolchain produces them.

## They were built inside the machine, and that is how you rebuild them

There is no cross-assembler. These are assembled by **CP/M's own `ASM.COM` and `LOAD.COM`**,
off the disk you are booting — and then `W` copies them straight back out to the host,
which is the same card this whole directory is about.

The source reaches the disk through the console, which is the one channel that exists
before any file-transfer utility does:

```
A> PIP R.ASM=CON:
   ...paste R.ASM...
   ^Z
A> ASM R                 -> R.HEX and R.PRN
A> LOAD R                -> R.COM
A> W R.COM               -> back out to the host, byte for byte
A> W R.HEX               -> text, by extension: it stops at the ^Z
A> W R.PRN
```

...with `hostdir` pointed at this directory. After the first one is built, `R.COM` itself is
how the other two sources get in, so nothing needs pasting twice.

**If you change a `.ASM`, rebuild and commit all three of its artifacts.** A checked-in
binary that nobody checks is a binary that rots — edit `R.ASM`, forget to rebuild, and
`R.COM` quietly goes on being the old program while every test still passes.

`tests/acceptance/hostbridge.cmake` enforces this, in two tests that check different halves
of the chain — and **only one of them runs by default**:

| test | proves | needs |
|------|--------|-------|
| `acceptance-hostbridge` | the committed `.HEX` still `LOAD`s to the committed `.COM`, and that `.COM` works | nothing — the disk is tracked |
| `acceptance-hostbridge-build` | the committed `.ASM` still *assembles* to the committed `.HEX`, `.PRN` and `.COM` | nothing — the same tracked disk |

Both tests boot the built-in `default` machine (a CompuPro Disk 1 and CP/M 2.2N) on a scratch
copy of `tests/media/compupro/CPM22N.DSK`. They erase files that they do not need from the copy
first, so that the 57 KB of source and what `ASM` makes fit on the disk. A fresh clone runs both.

Both modes then *use* what they built. So if a `.ASM` and the card ever disagree about the
protocol, the assembler still succeeds and the *transfer* fails — in the test, not in your
hands.

## The 8086 versions

`R.A86`, `W.A86` and `HDIR.A86` are the same programs for the 8088 side of the CompuPro CPU
8085/88, step for step and message for message. They use the same ports and the same BDOS
functions; the BDOS is `INT 224`, and the DMA buffer is set with functions 51 (segment) and 26
(offset). The user's view is in `docs/manual/file-transfer.md`.

One step has no 8080 twin: under MP/M-86 the three share the mutual exclusion queue `MXhostbr`,
so that two users do not mix their transfers on the board's one stream (`LOCK` and `UNLOCK` in
each source; `acceptance-hostbridge86-mpm` proves it). Under CP/M-86 and CP/M 8-16, function 12
says there is no MP/M, and `LOCK` does nothing.

They were built under CompuPro CP/M 8-16, which has DRI's `ASM86` and `GENCMD`:

```
A> R R.A86               (the 8080 R.COM through SW!, until an R.CMD exists)
A> ASM86 R               -> R.H86, R.LST and R.SYM
A> GENCMD R              -> R.CMD
A> W R.CMD
A> W R.H86               -> text, by extension
A> W R.LST
```

Two ASM86 rules that cost time: `[SI-1]` and `[SI+1]` are refused ("ILLEGAL INSTRUCTION
OPERANDS"); write `1[BX]`, or move the register. The data segment ends with a `DB 0` after the
stack, so that `GENCMD` keeps all of it (as in DRI's `RANDOM.A86`).

`tests/acceptance/hostbridge86.cmake` holds them to the committed files in the same two ways:

| test | proves |
|------|--------|
| `acceptance-hostbridge86` | each committed `.H86` still `GENCMD`s to the committed `.CMD`, and that `.CMD` moves files |
| `acceptance-hostbridge86-build` | each `.A86` still assembles to the committed `.H86`, `.LST` and `.CMD` (label `slow`, about a minute) |

Both boot `tests/media/compupro/cpm816-hb.toml` on a scratch copy of `CPM816.DSK`, and type
each command only after the `A>` prompt is back: `ASM86` takes a key typed ahead as a USER
BREAK. **If you change a `.A86`, rebuild and commit all three of its artifacts.**

## HB.INC is the master, and nothing includes it

DR's `ASM.COM` has no `INCLUDE` directive — that is M80's `MACLIB`. So each `.ASM` carries
its own copy of the ports, commands, status bits and error codes. `HB.INC` is where they
are *defined*: if a number in it and a number in a `.ASM` ever disagree, `HB.INC` is right
and the other is a typo.

## Every disk operation is a BDOS call

No BIOS entry points. No `IN`/`OUT` to a disk controller. No assumption about a DPB, a
sector size, a skew table, or which card the drive is on. The only ports these programs
touch are the bridge's own two.

That is why one `R.COM` runs on any CP/M 2.2 BIOS, whatever the disk controller. In
altairsim it ran unchanged on the MITS 88-DCDD, an 8 MB image and an 88-MDS minidisk; in
pascal100 the same `.COM` runs on CompuPro CP/M 2.2N, 2.2LD and 2.2K.

## If you move the card, reassemble

The port is an `EQU` at the top of each file. These programs deliberately do **not** scan
the bus looking for the bridge: a blind `IN` across 256 ports of an unknown machine would
take a byte out of a UART's receive register and step a floppy controller's head on the way
past. A program that guesses at hardware is a program that breaks other hardware.

They do check that the card is *there*, with `IDENT`, before doing anything — because an
`IN` from a port nobody decodes floats the bus and reads `0FFH`, and `0FFH` has `DAV` set.
Without that check, `R` on a machine with no bridge in it would cheerfully write a disk full
of `0FFH` bytes and call it a file.
