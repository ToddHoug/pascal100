# CompuPro CP/M-86 1.1 with a CADzilla graphics board

**To start it**, name the machine file from inside this folder:

```
$ pascal100 cadzilla86.toml
```

On **Windows** the program is `pascal100.exe`. The machine boots by itself. Two windows open. The
terminal window shows the CP/M prompt:

```
CP/M 86 1.1PAA
...
Loading CPM.SYS
...
NO M-DRIVE/H Active
Total Memory: 1 MEGABYTE

A>
```

The second window, `cad0`, is the monitor of the CADzilla board. It is black at 1024x768. It
shows a picture when a program starts the ACRTC and draws.

`cadzilla86-console.toml` is the same machine with the console on the terminal where you started
the program. Use it on a build with no window. On that build the CADzilla runs, but it shows
nothing.

`^E` (STOP) gives the keyboard back to the monitor at any time. `RUN` continues the guest.

**Go back to the `A>` prompt before you stop or quit.** A BIOS can keep written data in memory
for a short time ([Keep your disk safe](../../docs/manual/disks.md#go-back-to-the-a-prompt-before-you-stop)).

## The machine

This is the machine of the [`cpm86`](../cpm86/) example, with one more board. A CompuPro CPU 86/87
(an 8086, no 8087), an Interfacer 1 for the console, a Disk 1 floppy controller at `C0`, a System
Support 1 at `50` for its clock, the host bridge at `B0`, 1 MB of RAM, and a **CADzilla at `70`**.

**How it boots.** The 8086 starts at `FFFF0`. While the Disk 1 boots, its boot ROM answers every
memory read, so the 8086 runs the ROM's routine 0. The routine reads the boot tracks. The loader
on them loads `CPM.SYS` and starts CP/M-86. The startup line is `RUN FFFF0`, the 8086's reset
address. Over MCP, boot `cadzilla86-console.toml` with `run {from: 1048560, until: "A>"}`
(`from` takes a decimal number).

## The CADzilla

The CADzilla is a graphics board: a Hitachi **HD63484 ACRTC** with 2 MB of its own frame memory,
and a Brooktree **Bt453** color table. The 8086 never addresses a pixel. A program writes drawing
commands into the FIFO of the ACRTC, and the ACRTC draws. CP/M-86 does not know the board, so a
program drives it through its I/O ports:

| Port | What it is |
|---|---|
| `70` | The ACRTC address register (`OUT`) and status register (`IN`). Byte I/O. |
| `71` | The board's MODE register. `OUT` only. |
| `72`-`73` | The ACRTC data port: a register, or the FIFOs. Word I/O at `72`. |
| `74`-`77` | The Bt453: the address register, the color table, the address again, the overlay colors. |

**The board is set for 16-bit I/O** (`host_bus = "16"`, switch 6 of SW1). The ACRTC moves one
16-bit word for each access. A word `OUT DX,AX` or `IN AX,DX` with `DX = 72H` is one 16-bit bus
cycle: AL is the low byte, at port `72`, and AH is the high byte, at port `73`. For example, this
ASM86 fragment writes `093FH` to register `84H` (HDR):

```
        MOV     DX,70H
        MOV     AL,84H
        OUT     DX,AL           ; the address register: one byte
        MOV     DL,72H
        MOV     AX,093FH
        OUT     DX,AX           ; the whole register word: one 16-bit cycle
```

Use word I/O at port `72`. In 16-bit mode a byte `OUT` to `72` writes `FF` into the high byte of
the register. The monitor's `OUT` command is byte I/O. To program the board from the monitor,
set `host_bus = "8"` in the machine file. Then each access moves one byte, the high byte first.

`SHOW cad0` at the monitor shows the board's settings and its live values. The `wiring` line
shows `ok` when the ACRTC is set as the board is wired (8 bits per pixel, an address increment of
+8). The *User Manual* chapter on boards describes the board, and `docs/boards/cadzilla.md` in
the source gives every register.

## The host bridge

`R`, `W` and `HDIR` copy files between the CP/M disk and the folder you started pascal100 in.
They are the 8086 versions, `R.CMD`, `W.CMD` and `HDIR.CMD`
([Moving files in and out](../../docs/manual/file-transfer.md)). Use them to bring a program for
the CADzilla onto the disk: assemble it with `ASM86` and `GENCMD` on the disk.

## The files

| File | What it is |
|---|---|
| `cadzilla86.toml` | The whole machine. The console is a built-in terminal window. Needs a build with SDL3. |
| `cadzilla86-console.toml` | The same machine and disk, with the console on stdio. |
| `cpm86-11.dsk` | The system disk: CompuPro CP/M-86 1.1PAA, System Master disk 2 of 2, with `R.CMD`, `W.CMD` and `HDIR.CMD` added. A copy of the disk in [`cpm86`](../cpm86/), which lists the programs on it. |

**The disk is not under the MIT license.** CP/M-86 is Digital Research's, and the CompuPro files
are CompuPro's. Only `R.CMD`, `W.CMD` and `HDIR.CMD` are pascal100's, under the MIT license.

**There is no undo.** Drive 0 is read/write. Copy the image before you test writes.
