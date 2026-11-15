# CompuPro DISK 2 (`disk2`)

**Status:** done for what CompuPro's software uses: the registers, the five commands, DMA through
a [Selector Channel](compupro-selch.md), ATTN and its interrupt jumper, M20/M10 drives. A sector
is found at once (no rotation), and the image holds sectors, not headers. See *Limitations*.

## The real hardware

The CompuPro (Godbout) DISK 2, board 177 rev D (manual third edition, November 1983): an
"arbitrated 24 bit DMA hard disk controller for Fujitsu 2300 series drives". Up to four
Winchester drives: one 50-pin daisy-chained control cable (J5) and a 20-pin differential data
cable per drive (J1-J4). A finite state machine in two bipolar PROMs (U47/U48, 177P-1/-2) reads
and writes the headers and data fields; the board has no memory address logic of its own and
does nothing without a **Selector Channel** (board 172) on the bus.

CompuPro shipped it with the Fujitsu **M2302B** (20 MB, the DISK2 utility's default `m20`) and
the **M2301B** (10 MB, `m10`). Both have 244 cylinders, 8 or 4 heads and 12,000 unformatted
bytes a track. The drive's own sector switches cut the track into 11 slots of 1090 bytes, each
holding a 1024-byte sector and 45 bytes of format overhead.

Switches: SW1-2..5 DMA priority, SW1-6..10 sector size (one on), SW2-1..7 port A7-A1. Jumpers:
J6 write clock phase (C\* for Fujitsu/Memorex), J7-J14 ATTN to VI0-VI7, J15 ATTN to pINT.

## Sources

| Source | Path | Authority |
|---|---|---|
| DISK 2 & Selector Channel Technical Manual, third edition Nov 1983 | `reference/CompuPro Disk 2 & Selector Channel.md` | The registers, commands, status, theory of operation, track format. |
| Fujitsu M2301B/M2302B CE manual | (period PDF; facts quoted in the reference file) | 244 cylinders, heads, sector switch table (11 sectors = 1090 bytes). |
| CompuPro CP/M 8-16 V1.1R `COMPUPRO.EQU`, `TMXDISK2.DVR` | (period source) | The CTL byte values (`D2_READ` C8, `D2_WRT` D0, register selects), the step direction, the status test `(st & 7E) ^ 02`, port C8, Selector Channel F0, priority 10. |
| CompuPro MP/M 8-16 2.1E `XIOS20.MPM` | (period binary, disassembled) | It waits for ATTN as an interrupt on VI1 (8259 IR1, MP/M flag 9). |

Where the manual and the driver disagree, the driver won: page 10 calls WRITE FAULT "bit 2" of
STATUS; the bit table, the page 9 "status 3" rule and `D2_WRTF` all say bit 1.

## Register reference

| Port | OUT (write) | IN (read) |
|---|---|---|
| BASE (C8) | CTL | STATUS |
| BASE+1 (C9) | the data register CTL OP1/OP0 picks | one STEP pulse; no data (the bus floats) |

| Bit | CTL | STATUS |
|---|---|---|
| 7 | ATTN\*: 1 releases ATTN | ATTN\*: 0 = a command ended (STATUS latched) |
| 6 | RUN: 1 starts the command | TIME OUT |
| 5 | OP2 (also the step direction: 1 = in) | CRC ERROR |
| 4 | OP1 | OVER RUN |
| 3 | OP0 | READY\* |
| 2 | FAULT CLR | SEEK COMPLETE\* |
| 1 | US1 | WRITE FAULT\* |
| 0 | US0 | TRACK 0\* |

Data registers (RUN = 0, OP1/OP0): 00 DRIVE (bits 7-4 one-hot drive select, bits 3-0 head
lines), 01 CYLINDER, 10 HEAD, 11 SECTOR. CYLINDER/HEAD/SECTOR are only compared with the header.

Commands (RUN = 1, OP2-OP0): 0 NULL, 1 READ DATA, 2 WRITE DATA, 3 WRITE HEADER (3 bytes from
memory), 4 READ HEADER (3 bytes to memory). A good READ DATA leaves STATUS = 03 (02 on track 0).

## How it is simulated

- **Ports:** decodes IN and OUT at BASE and OUT at BASE+1. IN BASE+1 is taken by `snoop()`: it
  steps every selected drive that has a disk, in the OP2 direction, clamped to 0..243.
- **DMA:** a data command pulls pHOLD (`requestsBus()`) at `dma_priority`. Granted, the board's
  `Mover` finds a `selch` on the bus whose MODE byte serves that priority and has it run one
  cycle per byte (3 T-states each, no waits). The whole field moves at one instruction
  boundary, as the real board holds the bus for the data field. No matching Selector Channel:
  the bytes go nowhere and the command ends with OVER RUN.
- **No rotation** (the same rule as the uPD765): the command runs when the CTL write starts it,
  and NULL ends at once.
- **A seek takes 30 ms.** Each step pulse drops SEEK COMPLETE on the drive; it comes back 30 ms of
  emulated time after the last pulse. The M2302B buffers step pulses and seeks after the last one,
  and 30 ms is its minimum positioning time (Fujitsu spec table: 30 / 70 / 140 ms). The heads are
  on the new cylinder at once. Time is the emulated T-state clock, so at `clock_hz = 0` a seek
  costs almost no wall time.
- **Media:** a raw image of sectors only, `cylinder x heads x sectors x sector_size`, cylinder
  then head then sector, sectors from 0 (`DiskImage`, interleaved, `startSector` 0). M20 =
  21,987,328 bytes, M10 = 10,993,664. Any other size is refused with the expected size in the
  message (an XMODEM pad of up to 127 bytes is allowed). Each written sector goes to the host file
  at once (a sync per WRITE DATA), so a crash loses nothing the guest was told was written. An **empty file is a new disk**: it is
  filled with E5 to the full size, as the DISK2 utility's `data` pass leaves it.
- **Headers are not stored.** Every track is taken to hold CompuPro's headers (cylinder,
  head, 0..sectors-1). READ/WRITE DATA find the sector only when CYLINDER = the real cylinder
  (low 8 bits), HEAD = the head lines, SECTOR < sectors; otherwise TIME OUT. READ HEADER gives
  (cylinder, head, 0). WRITE HEADER takes any standard header (so any skew); another is logged
  and not kept.
- **Interrupt:** `interrupt = none | int | vi0..vi7` (J15, J7-J14), asserted while ATTN is.
- **Properties:** `port` (C8), `dma_priority` (10), `sector_size` (1024; only with no disk
  mounted), `interrupt` (none). Drives: `[[board.drive]]` with `unit`, `mount`, `readonly`,
  `create`, `type` (m20 | m10) and `sectors` (11); `type` and `sectors` are also MOUNT options.

### Reset

- `Reset::PowerOn`: registers cleared, the heads of every drive at cylinder 0 (a Fujitsu drive
  returns to zero at power on), faults cleared, ATTN asserted.
- `Reset::Bus`: the state machine stops (a hung command ends, a transfer is dropped), the result
  flags clear and ATTN is asserted (p. 3). The registers keep their contents (the manual does not
  say otherwise) and the heads stay where they are: the drives are not on the bus.

## Quirks reproduced

| Quirk | If you get it wrong |
|---|---|
| ATTN is asserted after RESET, and the STATUS register is latched while it is. | A driver that releases ATTN before it reads READY\* (TMXDISK2 `D2READY`) reads stale status. |
| ATTN on VI1 (J8) is how MP/M 8-16 2.1E's XIOS20 learns a command ended. | MP/M waits on flag 9 for ever. |
| US1/US0 in the command must name the drive DRIVE selects; otherwise TIME OUT. | A driver bug reads the wrong drive instead of failing. |
| A command to a drive with no disk never ends (the state machine runs on the drive's clock). | Software that forgot to check READY would see an error the real board never gives. |
| An IN from BASE+1 steps; it does not read. | A seek moves no heads, and every later READ DATA times out. |
| A step pulse drops SEEK COMPLETE\* until the seek ends. | CompuPro's DISK2.CMD steps once and waits up to 4,096 STATUS reads for SEEK COMPLETE\* to go high, then stops with "PANIC: 20 pin cable plugged in backwards" (SEEK COMPLETE comes over the 20-pin data cable). |
| The result flags (TIME OUT, CRC, OVER RUN) stay until the next command starts. | A transparent STATUS read after a failure looks clean. |
| A WRITE FAULT drive takes no command until FAULT CLR. | A read-only disk keeps working after a refused write, hiding the fault. |

## Limitations and deliberate departures

- **A command does not wait for SEEK COMPLETE.** On the real board the automatic functions are
  interlocked to it (p. 7): a command issued during a seek starts when the seek ends. Here it runs
  at once. The data is the same; CompuPro's drivers wait for SEEK COMPLETE before a command anyway.
- **Every seek takes 30 ms,** one cylinder or 243. The drive takes 30 to 140 ms by distance.
- **No rotation, no byte timing.** NULL does not wait for index, a command does not wait for its
  sector, and the DMA steals 3 T-states a byte, not the 1.7 ms a 1024-byte sector takes on the
  drive. Nothing that runs correctly can tell; a speed comparison with a real disk can.
- **Headers are not stored.** A program that writes non-standard headers (copy protection, a
  foreign format) loses them; it gets a log line. READ HEADER always gives sector 0.
- **OVER RUN is the only DMA failure,** and only for "no Selector Channel serves this priority".
  A WRITE DATA that ends that way leaves the disk unchanged; the real board would write
  whatever it had. A Selector Channel whose W/R\* is the wrong way runs its own cycle, as on the
  real pair: a read moves the sector nowhere, a write writes the floating bus (FF).
- **CRC ERROR never happens:** the image has no CRC to fail.
- **Operation codes 5-7** are not in the manual; they end with TIME OUT and a log line.
- **WRITE FAULT** comes only from a write to a read-only disk.
- **J6 (write clock phase)** has no effect.

## Verification

`tests/test_disk2.cpp` (86 checks) drives the board through bus cycles and a Z80 that issues the
command and polls STATUS as TMXDISK2 does: the port map, ATTN after reset and the interrupt
jumper, mounting (sizes, `type = m10`, a new E5 disk), drive status, stepping limits and the 30 ms
seek (it restarts on each pulse), READ
DATA through the Selector Channel to the right bytes and the counter, WRITE DATA read back (and synced to the host file at once), TIME
OUT for a missed seek / wrong head / sector 11, OVER RUN with the Selector Channel disabled, set
for another priority, or absent, the hang and the US1/US0 mismatch, WRITE FAULT and FAULT CLR,
WRITE HEADER / READ HEADER, the TMA priority check against a Disk 1, and SNAPSHOT / RESTORE.

CompuPro's own software, 2026-11-14, under CP/M-86 1.1PAA on the CPU 8085/88: `DISK2 M20 FORMAT
DATA` formatted all 244 cylinders with no hard or soft errors and reported "No bad sectors
detected"; the image is E5 throughout. (`DISK2 M20 ALL` ran its format pass and the start of its
verify pass cleanly; the full run, which the MP/M manual puts at 17 hours or more on real hardware,
was not completed.) Then MP/M 8-16 2.1F: `MPMGEN B:MPMLDRHD.85` and MPM20.SYS on a boot floppy,
`D:PIP A:=D:*.*[V]` to the M20 under MP/M, and MP/M boots from the hard disk with ATTN on VI1.

## References

- `reference/CompuPro Disk 2 & Selector Channel.md`
- [compupro-selch.md](compupro-selch.md)
