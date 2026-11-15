# CompuPro Interfacer 3

**Status:** done for the eight serial channels as software sees them. Synchronous mode, the
TxEMT/DSCHG option of channels 6 and 7, and the sync clock jumpers are not modeled (see
Limitations).

## The real hardware

The Interfacer 3 (CompuPro, later Viasyn, board 174 rev C, manual dated August 1985) is an
eight-channel serial board. Each channel is a 2651 USART with RS-232 drivers and receivers. One
5.0688 MHz crystal clocks the baud generators of all eight USARTs.

The eight channels share one block of eight I/O ports. A user select latch picks the USART that
the first four ports reach. Up to four boards can share one block, for 32 users.

Configuration in period:

- **S1-1:** ON takes the board off the bus.
- **S1-2..6:** A7-A3 of the block. ON is 0. CompuPro software uses 10H.
- **S1-7, S1-8:** the group of eight users that the board answers: 0-7, 8-15, 16-23 or 24-31.
- **J15, J16:** each channel's transmit and receive interrupts, to any VI line.
- **J17:** 0, 1 or 2 wait states on USART accesses.
- **J1, J2:** channels 6 and 7 as DTE or DCE. Channels 0-5 are DCE only.
- **J18:** clear on SLAVECLR\* (factory) or POC\*.

## Sources

| Source | Path | Authority |
|---|---|---|
| *Interfacer 3 Technical Manual*, Viasyn, 8261-0015, August 1985, 46 pp. with the logic diagram | `reference/CompuPro Interfacer 3.md` (converted from `CompuPro Interfacer 3 manual.pdf`) | Every fact on this page. Ports p. 2, user select pp. 3-4, interrupts pp. 6-8, 2651 registers pp. 8-11, decode pp. 15-16, logic diagram pp. 31-36 |
| *MP/M 8-16 Technical Manual and Installation Procedures*, CompuPro, March 1984 | `Documentation/CompuPro/MPM_Technical_Manual_and_Installation_Mar84.pdf` (project folder) | The jumpers MP/M expects: J15 all TxINT to VI3, J16 all RxINT to VI2 (p. 37) |

The manual's text has two errors that the logic diagram corrects:

- Page 13 gives channels 0-5 the "master" (DTE) pin numbers. The logic diagram wires them as DCE,
  as page 12 says.
- Page 3's user table stops at user 6. The latch decodes all eight.

The CP/M 8-16 1.1R BIOS source (`TMXINTIO.INI`) comments VI2 as "Interfacer 1-4 transmit" and
VI3 as "receive". The MP/M 8-16 manual jumpers transmit to VI3 and receive to VI2. MP/M 8-16 2.1E
on this board confirms the manual: its VI2 handler reads the receive status port.

## Register reference

Eight ports at `port` (a multiple of 8). "Selected" means the USART that D0-D2 of the user select
latch pick. Ports 0-5 answer only while D3-D4 of the latch match the board's `users` switch.

| Relative | OUT (write) | IN (read) |
|---|---|---|
| 0 | Transmit data, selected USART | Received data, selected USART. Clears RxRDY |
| 1 | SYN1/SYN2/DLE (synchronous mode) | 2651 status: D0 TxRDY, D1 RxRDY, D2 TxEMT/DSCHG, D3 PE, D4 OE, D5 FE, D6 DCD, D7 DSR |
| 2 | Mode registers 1 then 2 | Mode registers 1 then 2 |
| 3 | Command register | Command register |
| 4 | Transmit interrupt mask: bit *k* = user *k* | TxRDY of users 0-7, whatever the mask |
| 5 | Receive interrupt mask: bit *k* = user *k* | RxRDY of users 0-7, whatever the mask |
| 6, 7 | User select: D0-D2 user, D3-D4 board | Not decoded |

## How it is simulated

- **Units:** `user0` to `user7`, one 2651 each. The USART is the `Sig2651` chip that the System
  Support 1 also uses.
- **The user select latch** is taken in `snoop()`, so every board at the same port latches each
  select write. That is what the real latch U34 does: its clock needs only the address compare.
  A board's `decodes()` follows the match between D3-D4 and its `users` switch, and the board
  calls `decodeChanged()` when that match changes.
- **The select write itself** is decoded by the board that is selected when the write arrives.
  So exactly one board claims it, and the bus does not report a contention.
- **Interrupts:** TxINT*k* = TxRDY*k* AND mask bit *k*; RxINT*k* = RxRDY*k* AND mask bit *k*. Each
  goes to the VI line set by `tx_int` or `rx_int`. A board that is not selected still drives its
  interrupts.
- **Wait states:** `waits` adds 0, 1 or 2 wait states to each access of ports 0-3.
- **RS-232 inputs:** the real board pulls CTS, DSR and DCD up to +12V, so an unconnected input is
  ON. The 2651 model reads DSR and DCD as asserted, which agrees.

Properties:

| Property | Switch | Default | Meaning |
|---|---|---|---|
| `port` | S1-2..6 | 10 | The base of the 8-port block |
| `enabled` | S1-1 | on | The board answers |
| `users` | S1-7, S1-8 | 0-7 | The group of users the board answers |
| `waits` | J17 | 0 | Wait states on a USART access |
| `select`, `tx_mask`, `rx_mask` | | | Read-only: the latch and masks as last written |
| `user<n>.baud` | | 9600 | The line rate until the guest writes Mode Register 2 |
| `user<n>.tx_int`, `user<n>.rx_int` | J15, J16 | none | The VI line for the user's TxINT and RxINT |
| `user<n>.connect` | | null | The endpoint on the user's line |

### Reset

`Reset::PowerOn` and `Reset::Bus` do the same thing. CLR clears the user select latch (user 0,
board 0) and both masks, and drives the RESET pin of each USART. The lines stay connected.

## Quirks reproduced

| Quirk | If you get it wrong |
|---|---|
| Every board at the same port latches every user select write | A second board keeps an old selection and answers when it should not. Two boards drive the bus together |
| Ports 0-5 answer only while D3-D4 match the board's switch | A driver that polls users 8-15 reads board A's USARTs |
| The status ports 4 and 5 ignore the masks | A driver that polls port 5 with the interrupts masked finds nothing to do |
| A0 is not decoded at the user select port | A driver that writes the select to port 6 changes nothing |
| An unconnected DSR or DCD reads asserted | The CP/M 8-16 BIOS Interfacer 3 console waits for DSR forever |

## Limitations and deliberate departures

- **Synchronous mode** is not modeled. A write to port 1 (SYN1/SYN2/DLE) is ignored.
- **TxEMT/DSCHG on channels 6 and 7** (J3/J4, J13/J14) is not modeled. The interrupt jumpers
  carry TxRDY and RxRDY only.
- **The sync clock jumpers** (J5-J12) and **J1/J2** have no effect. A stream has no clock lines,
  and it does not care which pins TxD and RxD use.
- **A select write to a board number with no board** is decoded by nobody. The real boards
  latch it all the same; here they do too, through `snoop()`. Only the bus's "unclaimed" report
  differs.
- **CTS** does not gate the transmitter. The stream's own flow control stands in for it.

## Verification

`tests/test_interfacer3.cpp` (59 checks), two boards at 10H on a real bus:

- After reset board A answers; an IN from the select port is not decoded.
- Both boards latch a select write, the selected board changes, and the bus reports no
  contention. Port 16 selects as port 17 does; only D0-D4 are latched.
- Programming, sending and receiving through the selected USART, on board A and board B.
- Ports 4 and 5 show TxRDY and RxRDY whatever the masks; a mask bit puts TxINT on VI3 and RxINT
  on VI2; reading the byte drops RxINT.
- An unselected board raises its interrupt.
- Reset clears the latch and the masks; a snapshot keeps the latch; CONFIG SAVE writes the
  settings back; a port that is not a multiple of 8 is refused.

Period software: MP/M 8-16 2.1E, with two boards at 10H (users 0-7 and 8-15), all TxINT on VI3,
all RxINT on VI2, the Disk 1 on VI4 and the SS1 console. After `logout` at the console, user 7
(MP/M's tty1, on a socket) shows its own `0A>` prompt and runs `DIR` and `MPMSTAT`.

## References

- `reference/CompuPro Interfacer 3.md`
- `docs/boards/compupro-ss1.md`, the other board with a 2651.
- `docs/boards/compupro-interfacer1.md`
