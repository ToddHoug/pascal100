# CompuPro MP/M 8-16 2.1F from a hard disk

**To start it**, name the machine file from inside this folder:

```
$ pascal100 mpm816.toml
```

On **Windows** the program is `pascal100.exe`. The machine boots by itself. Three terminal windows
open: **MP/M console**, **tty1** and **tty2**. Each one is a user of the same MP/M system and shows
the prompt:

```
CompuPro MP/M 8-16
0A>
```

The `0` in `0A>` is the user number. There is no password file, so no user is asked to log in.

`mpm816-console.toml` is the same machine with the console on the terminal where you started the
program, and with tty1 and tty2 not connected. Use it on a build with no window.

`^E` (STOP) gives the keyboard back to the monitor at any time. `RUN` continues the guest.

## Type SYNC before you stop

MP/M keeps a write cache for the hard disk. It writes the cache to the disk every 30 seconds, when
a user types `LOGOUT`, and when a user types `SYNC` (*MP/M 8-16 Technical Manual*, p. 20, "The SYNC
command"). If you stop the simulator before one of these, the last writes are lost, as on the real
machine when you switch it off. A file can then be in the directory with no data, or not be there.

```
0A>SYNC
```

The prompt comes back before MP/M has written the cache. A person who then stops the simulator
is slow enough. A script must let the machine run for about a second after the prompt (for
example, a `run` with no input) before it stops the simulator.

## Which CompuPro system is this?

CompuPro sold three operating systems for the CPU 8085/88 card. This example is the third:

| System | Example | Users | Runs |
|---|---|---|---|
| CP/M-86 | [`cpm86`](../cpm86/) | one | 8086 programs (`.CMD`) only |
| CP/M 8-16 | [`cpm8-16`](../cpm8-16/) | one | 8086 programs (`.CMD`) and 8080 programs (`.COM`) |
| **MP/M 8-16** | `mpm8-16` | several | 8086 and 8080 programs, each user at a terminal |

MP/M 8-16 is CompuPro's version of Digital Research's MP/M-86. MP/M runs on the 8088. Like CP/M
8-16, it has CompuPro's `SW!`, which runs an 8080 `.COM` program on the 8085.

## The machine

A CompuPro CPU 8085/88, a System Support 1 (the console's serial port, and the interrupt
controllers), a Disk 1 floppy controller at `C0`, a Disk 2 hard disk controller at `C8` with a
Selector Channel at `F0`, an Interfacer 4 for tty1, tty2 and the two printers, the host bridge at
`B0`, and 1 MB of RAM. The
board settings are the ones in the *MP/M 8-16 Technical Manual* for one Interfacer 4.

**How it boots.** The Disk 1 boot ROM (routine 6) reads the boot floppy in drive 0. Its loader is
CompuPro's hard-disk loader ("Hard Disk MP/M 8-16 Loader Rev. 1.0D."). It reads `MPM.SYS` from the
floppy, and MP/M then runs from the hard disk. Keep the floppy in drive 0. The hard disk is drives
`A:`, `B:` and `C:`. The floppies are `D:` and `E:`.

**The printers.** Printer 0 is the Interfacer 4's Centronics port. Its output goes to
`printer0.txt` in this folder. `PRINTER 1` selects printer 1, the Interfacer 4's serial port 5,
which writes `printer1.txt`. Try `PIP LST:=TTYS`.

**The host bridge.** `R`, `W` and `HDIR` copy files between the hard disk and the folder you
started pascal100 in. They are the 8086 versions, `R.CMD`, `W.CMD` and `HDIR.CMD`, on `A:` with the
SYS attribute, so every user number finds them
([Moving files in and out](../../docs/manual/file-transfer.md)). **One user at a time:** the board
has one transfer stream, so when another user has it, the utilities wait their turn.

## The files

| File | What it is |
|---|---|
| `mpm816.toml` | The whole machine: the console, tty1 and tty2 in built-in terminal windows. Needs a build with SDL3. |
| `mpm816-console.toml` | The same machine and disks, with the console on stdio. |
| `mpm816-boot.dsk` | The boot floppy: a copy of the MP/M 8-16 2.1F master disk 1 (serial M86-272-0972) with the hard-disk loader on its system tracks (`MPMGEN MPMLDRHD.85`) and `MPM.SYS` = `MPM20.SYS`. |
| `m20.dsk` | The hard disk: a Fujitsu M2302B ("M20", 244 cylinders, 8 heads, 11 sectors of 1024 bytes, 21,987,328 bytes). |

**How the hard disk was made**, as in the *MP/M 8-16 Technical Manual*: CompuPro's `DISK2`
formatted it (p. 7; `DISK2 M20 FORMAT DATA`, under CP/M-86), and `MPMGEN B:MPMLDRHD.85` made the
boot floppy (p. 9). Then, under MP/M from the boot floppy (p. 11): `D:PIP A:=D:*.*[V]`, `PIP` of the `TTYS` and `LPRS` files for one Interfacer 4, `SET *.CMD
[SYS,RO]`, and `SYNC`. `TTYS` starts with `MULTI`, so all three terminals start at once. Later, `R.CMD`, `W.CMD` and
`HDIR.CMD` were copied to `A:`, with `SET [SYS,RO]` and `SYNC`.

Only Digital Research and CompuPro files, and the three host bridge utilities, are on the disks. Unused blocks and deleted directory
entries are filled with `E5`, so no old data is on them.

**The disks are not under the MIT license.** MP/M-86 is Digital Research's, and the CompuPro files
are CompuPro's. Only `R.CMD`, `W.CMD` and `HDIR.CMD` are pascal100's, under the MIT license.

**There is no undo.** Both disks are read/write. Copy them before you test writes.
