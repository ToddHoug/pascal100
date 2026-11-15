# UCSD Pascal III.0 on the Pascal-100

**To start it**, name the machine file from inside this folder:

```
$ pascal100 pascal.toml
```

On **Windows** the program is `pascal100.exe`:

```
> pascal100.exe pascal.toml
```

A VT52 window opens, and Pascal starts in it at once. You do not type a boot command. The
`Command:` line is at the top of the screen:

```
Command: E(dit, R(un, F(ile, C(omp, L(ink, X(ecute, D(ebug ? [III.F1-2.0]
Welcome  WORK,  to
U.C.S.D.  Pascal  System  III.0
Current date is  15-Jul-81
```

Type a letter at the `Command:` line. For example, `F` starts the Filer, `L` in the Filer lists a
volume, and `*` names the boot volume. `Q` leaves the Filer.

The whole machine is in `pascal.toml`, one board after the other: the Digicomp Pascal-100 card (a Z80 and the
Western Digital MicroEngine), a CompuPro Interfacer 1 for the console, a CompuPro Disk 1 floppy
controller and 128K of RAM. `RUN 0` is the whole startup of the machine file. The boot EPROM of the
Disk 1 reads track 0 of drive 0, the Z80 loads the Pascal boot and BIOS, and the MicroEngine starts
the Pascal system.

## The monitor and how to stop

The `pascal100>` prompt stays in the terminal where you started the program. `^E` (STOP) gives the
keyboard back to the monitor. `RUN` continues Pascal.

**To finish, type `QUIT` at the monitor.** To restart Pascal, start the program again. `RESET`
followed by `RUN 0` does not reliably restart Pascal. The real Pascal-100 does the same.

## Without a window

`pascal.toml` needs a build with SDL and a display. On a build with no window, use the other file:

```
$ pascal100 pascal-console.toml
```

It is the same machine and the same disk, with the console on the terminal where you started the
program. **The screen codes are VT52.** A terminal that is not set to VT52 shows the `Command:`
line, but it cannot draw the screen of the editor. To use a VT52 terminal program of your own on
macOS or Linux, type `CONNECT if1:a pty` at the monitor, and open the name that it prints.

## The disk

`pascal-dd.dsk` is a double-density UCSD Pascal III.0 system disk, built for this hardware from the
two Digicomp Research distribution disks. It has the compiler, the editor, the Filer, the linker,
the library and the utilities. It passed on a real Pascal-100 on 2026-11-06.

**Drive 0 is read/write.** When you save a file in Pascal, the image changes. Keep a copy of the
image if you want to go back to the original.

The disk holds UCSD Pascal III.0 and the Digicomp Research Pascal-100 software. **The MIT license
of pascal100 does not cover the disk.** To build the disk yourself from the distribution disks,
follow the Pascal-100 build guide, `HOWTO-Build-the-System.md`.

## The files

| File | What it is |
|---|---|
| `pascal.toml` | The whole machine in one file: the Pascal-100 card, an Interfacer 1, a Disk 1 and 128K, the disk in drive 0, `RUN 0`, and the console in a **built-in VT52 window**. Needs a build with SDL3. |
| `pascal-console.toml` | The same machine and disk, with the console on stdio. |
| `pascal-dd.dsk` | The system disk: UCSD Pascal III.0, double density, VT52 screen codes. |
