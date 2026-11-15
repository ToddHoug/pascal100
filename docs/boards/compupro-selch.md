# CompuPro Selector Channel (`selch`)

**Status:** done for DMA: the programming sequence, memory and I/O cycles, up/down counting, the
priority match. The boot EPROM is not modeled.

## The real hardware

The CompuPro (Godbout) Selector Channel, board 172 rev D (documented in the DISK 2 manual,
November 1983). An IEEE 696 board that provides "all of the bus address, status and control
strobes required by a temporary master". A device controller (the [DISK 2](compupro-disk2.md))
arbitrates for the bus; the Selector Channel watches the arbitration, and when the winner's
priority matches the one it was programmed for, it takes the bus from the CPU and generates the
cycles the controller asks for with pSYNC. Several controllers can share one Selector Channel,
one at a time. It also carries an optional 2716 boot EPROM over PHANTOM\*.

Switches: SW1-1 boot EPROM enable, SW1-2 one wait state on every DMA cycle, SW1-3..10 the port
(A7..A0, ON = 0). Jumpers: J1-J4 EPROM wait states, J8-J10 EPROM address (bus, ground or +5 V).

## Sources

| Source | Path | Authority |
|---|---|---|
| DISK 2 & Selector Channel Technical Manual, Nov 1983, pp. 31-41 | `reference/CompuPro Disk 2 & Selector Channel.md` | Programming, MODE bits, theory of operation. |
| The same manual, logic sheet 1 | (period PDF) | SW1-3 = A7 ... SW1-10 = A0, ON = 0 (not in the text). |
| CompuPro `COMPUPRO.EQU`, `TMXDISK2.DVR`; MP/M 8-16 2.1E `XIOS20.MPM` | (period source and binary) | Port F0; the sequence IN, A23-A16, A15-A8, A7-A0, MODE; MODE A5 (read) / 25 (write) for priority 10. |

## Register reference

| Port | OUT (write) | IN (read) |
|---|---|---|
| PORT (F0) | the next byte of the program | disables the board; no data (the bus floats) |

After the IN, exactly four OUTs: A23-A16, A15-A8, A7-A0, MODE. The board is enabled after the
fourth.

| MODE bit | Name | Meaning |
|---|---|---|
| 7 | W/R\* | 1 = write to the slave (disk to memory), 0 = read |
| 6 | IO/M\* | 1 = I/O cycles (the address does not count), 0 = memory |
| 5 | U/D\* | 1 = count up, 0 = down, after each memory byte |
| 4 | WAIT | one wait state per cycle |
| 3-0 | DMA3\*-DMA0\* | the complement of the controller's priority |

## How it is simulated

- **Port:** decodes OUT only. The IN is seen by `snoop()`: it clears the write count and
  disables the board, and the bus floats the data (the real board drives nothing).
- **The shift register:** each OUT shifts MODE -> low -> high -> extended (p. 33), so the byte
  order falls out of the hardware; a fifth OUT shifts again. Enabled after four OUTs since the
  last IN or reset.
- **It never asks for the bus** (no `busMaster()`), so it is not a TMA master and never conflicts.
  The controller is the master: it calls `serves(priority)` to find the board and `cycle()` for
  each byte. `cycle()` does a physical memory cycle at A23-A0 (`memReadX`/`memWriteX`) or an I/O
  cycle at A15-A0, as MODE says, then counts a memory address up or down (24-bit wrap).
- **Properties:** `port` (F0).

### Reset

- `Reset::PowerOn`: disabled, address and MODE zero.
- `Reset::Bus`: disabled (U10 cleared); the address and MODE keep their contents.

## Quirks reproduced

| Quirk | If you get it wrong |
|---|---|
| The IN must come first; the board powers up disabled. | A driver that skips the IN still works in the model and not on the board. |
| MODE's low nibble is the **complement** of the priority (A5 for priority 10). | The DISK 2 gets no cycles: OVER RUN. |
| The address is not reset between commands. | WRITE HEADER relies on it: each command takes the next 3 bytes of the header table. |
| I/O cycles do not count. | A DMA to a port would walk across the I/O space. |

## Limitations and deliberate departures

- **The boot EPROM** (SW1-1, PHANTOM\*, J1-J4, J8-J10) is not modeled. CompuPro's MP/M 8-16 runs
  with SW1-1 OFF and boots its hard disk from a floppy.
- **Wait states** (SW1-2, MODE D4) are not modeled: no slow memory, nothing to wait for.
- **The overlap of masters** (XFER1/XFER2) and pSTVAL\*/strobe timing are not modeled; a cycle
  is one bus operation.

## Verification

`tests/test_disk2.cpp`: the four-OUT sequence and byte order, the IN that disables, RESET, down
counting, an I/O cycle that does not count, the priority match (A5 vs A4), and every DISK 2
transfer through it.

## References

- `reference/CompuPro Disk 2 & Selector Channel.md`
- [compupro-disk2.md](compupro-disk2.md)
