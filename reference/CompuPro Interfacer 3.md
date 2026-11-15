# CompuPro Interfacer 3 — Technical Manual

Converted from `Documentation/CompuPro/CompuPro Interfacer 3 manual.pdf` (46 pages, a scan with
no OCR text layer). Viasyn Corporation, part no. 8261-0015, board no. 174 rev C, first edition
April 1981, latest printing August 1985. Converted 2026-11-13.

Conversion method: every page was read from its page image. There is no OCR layer to compare.

Conversion notes:

- **Printed page = PDF page − 4** for the numbered pages. PDF page 2 is blank.
- **Printed page 1 is missing from the scan.** It holds "About Interfacer 3" and the start of
  "Technical Overview" (see the contents). PDF page 6 is printed page 2, and it starts in the
  middle of a sentence.
- Apparent errors in the printed manual are kept as printed and marked **[sic]**, with a short
  note.
- Photographs are described, not reproduced.

---

## Cover (PDF page 1)

VIASYN (logo). COMPUPRO INTERFACER 3™ TECHNICAL MANUAL. The cover art is a drawing of PC board
traces. Price "$20.00" at the lower right.

PDF page 2 is blank.

## Title page (PDF page 3)

INTERFACER 3 Technical Manual, IEEE 696/S-100. A photograph of the board (the edge reads
"INTERFACER 3" and "CompuPro"). Caption: "8 CHANNEL SERIAL I/O BOARD, RS-232 with full
handshake".

## Copyright and disclaimer (PDF page 4)

INTERFACER 3 TECHNICAL MANUAL. Copyright 1981, 1982, 1983, 1984, 1985 Viasyn Corporation,
Hayward, CA 94545.

First Edition: April 1981. Latest Printing: August 1985.

Part No: 8261-0015. Filename: INTER3U.DOC. Board No: 174 Rev: C.

DISCLAIMER - Viasyn Corporation makes no representations or warranties with respect to the
contents hereof and specifically disclaims any implied warranties of merchantability or fitness
for any particular purpose. Further, Viasyn reserves the right to revise this publication and to
make any changes from time to time in the content herof [sic] without obligation of Viasyn to
notify any person of such revision or changes.

Registred [sic] trademark: CompuPro; Viasyn Corporation.

Trademark: TRI-STATE; National Semicondutor [sic] Corp.

All rights reserved. No part of this publication may be reproduced or transmitted in any form,
or by any means, without the written permission of Viasyn. Printed and assembled in the United
States of America.

## Contents (PDF page 5)

| Section | Page |
|---|---|
| About Interfacer 3 | 1 |
| Technical Overview | 1 |
| Port Map | 2 |
| Port Addressing | 2 |
| User/Board Selection | 3 |
| Wait State Selection | 4 |
| Cables | 4 |
| Slave Clear/Power-On-Clear Option | 5 |
| Using Interrupts | 6 |
| Channel 6/7 Interrupt Option | 6 |
| Interrupt Control Registers | 7 |
| Interrupt Status Registers | 7 |
| USART Initialization | 8 |
| Data Registers | 9 |
| Status Register | 9 |
| Mode Registers | 10 |
| Command Register | 11 |
| Serial Mode Jumpers | 12 |
| RS-232 Control Lines | 13 |
| Synchronous Mode Clock Driver/Receivers | 14 |
| THEORY OF OPERATION | 15 |
| S-100 Bus Drivers | 15 |
| I/O Port Decode Logic | 15 |
| Strobe Generation Logic | 16 |
| Wait State Logic | 16 |
| Interrupt Control/Status Logic | 16 |
| USARTs | 17 |
| RS-232 Level Conversion Logic | 17 |
| USER NOTES | 18 |
| SAMPLE TEST PROGRAMS: Asynchronous Mode | 19-20 |
| Synchronous Mode | 21-22 |
| CP/M Console | 23-24 |
| INS2651 Programmable Communications Interface | 25-30 |
| LOGIC DIAGRAM | 31-32 |
| USARTs 0 - 7 (detail) | 33-36 |
| PARTS LIST | 38 |
| COMPONENT LAYOUT | 39 |
| CUSTOMER SERVICE/LIMITED WARRANTY INFORMATION | 40 |

## About Interfacer 3 and Technical Overview (page 1)

**Not in the scan.** Page 2 starts with the end of the overview:

> ...connected in either DTE mode or DATA COMMUNICATION EQUIPMENT (DCE) mode. This allows direct
> connection to all types of RS-232 equipment including modems. In addition, these two channels
> are capable of high speed synchronous operation using internal or external clocks.

Conversion note: "these two channels" are probably relative users 6 and 7, the two channels with
standard 26-pin cables, full handshake and the synchronous clock option (see "Cables", "Channel
6/7 Interrupt Option" and "Synchronous Mode Clock Driver/Receivers"). The missing page would
say so; this is an interpretation.

## Port Map (page 2)

The INTERFACER 3 interface uses a block of eight port addresses for communication between it and
the host processor. The address of the first port is switch selectable to any address which is a
multiple of eight. The ports will be referred to as relative ports 0 - 7.

| Relative port | Function |
|---|---|
| 0 | USART Data Register (R/W) |
| 1 | USART Status Register (R); SYN1/SYN2/DLE Register (W) |
| 2 | USART Mode Register (R/W) |
| 3 | USART Command Register (R/W) |
| 4 | Transmit Interrupt Status Register (R); Transmit Interrupt Mask Register (W) |
| 5 | Receive Interrupt Status Register (R); Receive Interrupt Mask Register (W) |
| 6 | Not used |
| 7 | User Select Register (write only) |

## Port Addressing (page 2)

DIP switch S1, positions 1 thru 6 are used to select the base address of the eight port block in
a binary fashion as shown below:

| Switch position | Address bit |
|---|---|
| 1 | PORT DISABLE WHEN "ON" |
| 2 | A7 |
| 3 | A6 |
| 4 | A5 |
| 5 | A4 |
| 6 | A3 |

"ON" = "0", "OFF" = "1".

EXAMPLE: To address this board at addresses 10H thru 17H for CompuPro operating systems,
position 1 and 5 would be "OFF" and positions 2 thru 4 and positions 6 would be "ON".

EXAMPLE: To address this board at addresses 38H thru 3FH, positions 1, 4, 5, and 6 would be
"OFF" and positions 2 and 3 would be "ON".

## User/Board Selection (page 3)

To select a particular channel and to select which board that channel will be on (when running
more that [sic] 8 users), requires the use of the User Select Port and two board select switches.
The five bit User Select Register determines which of 32 possible users will be selected at a
particular time. The two board select switches determine whether a board will respond to users 0
thru 7, 8 thru 15, 16 thru 23, and 24 thru 31. A particular user (0-31) is selected by
outputting the five bit number that represents that user. The diagram shown below will describe
the relation between the board select switches and the User Select Register.

USER SELECT REGISTER

| Data bit | Name | Function |
|---|---|---|
| D0 | US0 | USER SELECT 0 (LSB) |
| D1 | US1 | USER SELECT 1 |
| D2 | US2 | USER SELECT 2 (MSB) |
| D3 | BS0 | BOARD SELECT 0 (LSB) |
| D4 | BS1 | BOARD SELECT 1 (MSB) |
| D5 | | NOT USED |
| D6 | | NOT USED |
| D7 | | NOT USED |

Since each INTERFACER 3 will support 8 users, we will refer to these eight as RELATIVE USER 0-7.
These eight ports are physically configured with relative user 0 on the extreme right side of
the board and relative user 7 on the extreme left side. To determine the exact user number, the
RELATIVE USER number must be added to the USER OFFSET number. The RELATIVE USER number
corresponds to the three bits above called USER SELECT 0-2, and the USER OFFSET number
corresponds to the two bits above called BOARD SELECT 0 and 1. These five bits determine the
exact user number.

| US2 | US1 | US0 | Relative user number |
|---|---|---|---|
| 0 | 0 | 0 | USER 0 |
| 0 | 0 | 1 | USER 1 |
| 0 | 1 | 0 | USER 2 |
| 0 | 1 | 1 | USER 3 |
| 1 | 0 | 0 | USER 4 |
| 1 | 0 | 1 | USER 5 |
| 1 | 1 | 0 | USER 6 |

**[sic]** The printed table stops at USER 6. The row 1 1 1 = USER 7 is missing.

| S1-8 | S1-7 | BS1 | BS0 | User offset |
|---|---|---|---|---|
| ON | ON | 0 | 0 | 0 |
| ON | OFF | 0 | 1 | 8 |
| OFF | ON | 1 | 0 | 16 |
| OFF | OFF | 1 | 1 | 24 |

EXAMPLE (page 4): To address the INTERFACER 3 to respond to users 0 thru 7, switches S1-7 and
S1-8 would be "ON". To select a particular user in the group from 0 to 7, BS1 and BS0 must be "0"
for the board to respond. To select user 5, a 05H must be sent to the user select port.

EXAMPLE: To address the INTERFACER 3 to respond to users 16 thru 23, switch S1-7 would be "ON",
and switch S1-8 would be "OFF". To select a particular user in the group from 16 to 23, BS1 must
be a "1" and BS0 must be "0" for the board to respond. To select user 18, a 12H must be sent to
the user select port.

## Wait State Selection (page 4)

The INTERFACER 3 was designed to run in very fast microcomputer systems by allowing up to two
wait states to be added when accessing the USART registers. Since the user select and interrupt
control registers are capable of higher speed operation than the USART registers, no wait states
are inserted even when they are enabled on the board.

The three vertical pins at J17 control the enabling of one or two wait states. With the black pin
shunt connecting pins "A" and "C", one wait state will be inserted. With the pin shunt connecting
pins "B" and "C", two wait states will be inserted. If the pin shunt is left removed, no wait
states will be inserted.

## Cables (pages 4-5)

The INTERFACER 3 is designed to use two each of two different cables assemblies [sic]. Relative
users 0-5 use a custom 50 conductor cable and relative users 6 and 7 use standard 26 conductor
cables identical to those used on the INTERFACER 1 and INTERFACER 2.

Relative users 0-2 (50 pin connector on the far right) and relative users 3-5 (50 pin connector
in middle of board) use a custom three-user cable (see photo A page 7). This cable consists of a
female 50 pin insulation displacement connector that splits into thirds and connects to three
female DB-25 connectors. The actual cable has positions 1-16 (pin 1 on the far left side of the
connector) on the first DB-25, positions 17-32 on the second DB-25, and positions 33-50 on the
third DB-25.

NOTE: The pin numbers on the circuit diagram show the pin numbers on the DB-25 connector and not
the 50 pin connector.

Relative user 7 (26 pin connector on the far left) and relative user 6 (26 pin connector to the
right of user 7) use standard RS-232 I/O cables (see photo B below). This cable consists of a
female 26 pin insulation displacement connector that mates to a female DB-25 (the 26th conductor
is not used).

NOTE: The pin numbers on the circuit diagram show the pin numbers on the DB-25 connector and not
the 26 pin connector.

Page 5 has two unlabelled photographs: the top one is a 50-pin ribbon split three ways to three
DB-25 connectors; the bottom one is a 26-pin ribbon to one DB-25. **[sic]** The text calls them
"photo A page 7" and "photo B below"; both are on page 5.

## Slave Clear/Power-On-Clear Option (page 5)

The INTERFACER 3 is designed to be cleared by either pRESET\* or SLAVECLR\*. In some older
non-IEEE 696 processor boards, POC\* does not generate SLAVECLR\* and pRESET\*. On these systems
this board might not be cleared upon power-up. By cutting the trace at J18 between "B" and "C",
and installing a jumper between holes "A" and "C", this board will be cleared by POC\* instead of
SLAVECLR\*.

## Using Interrupts (page 6)

The INTERFACER 3 has a simple but elegant interrupt structure that allows considerable
flexibility. Each USART generates both a transmit and receive interrupt, for a total of 16
distinct interrupts for the board. A transmit interrupt indicates that the USART transmit
register is empty and it is ready to accept a character. A receive interrupt indicates that data
is available from the receiver data register. Each of these interrupts may be masked "OFF" or
"ON" by altering the INTERRUPT CONTROL REGISTERS as described below. Each of these interrupts
are open collector, and may be individually tied to any of the eight vectored interrupt lines
(VI0-VI7). The status of each interrupt line may be sampled by reading the INTERRUPT STATUS
REGISTERS as described below.

Since each of the 16 interrupts generated on the INTERFACER 3 may be tied to any of the eight
vectored lines, almost any type of priority scheme may be implemented. All transmit interrupts
are brought out on one side of jumper socket J15, and all receive interrupts are brought out on
one side of jumper socket J16. On the opposite side of each socket, each of the eight vectored
interrupt lines are brought out. By using the provided headers, any USART interrupt may be
connected to any VI line. The pin-out of J15 and J16 are shown below.

The drawing shows each socket as two columns of pins with the sources on one side and the VI
lines on the other:

| J15 source | J15 pin | J15 pin | VI line | J16 pin | J16 pin | J16 source |
|---|---|---|---|---|---|---|
| TxINT 0 | 9 | 8 | VI0 | 9 | 8 | RxINT 0 |
| TxINT 1 | 10 | 7 | VI1 | 10 | 7 | RxINT 1 |
| TxINT 2 | 11 | 6 | VI2 | 11 | 6 | RxINT 2 |
| TxINT 3 | 12 | 5 | VI3 | 12 | 5 | RxINT 3 |
| TxINT 4 | 13 | 4 | VI4 | 13 | 4 | RxINT 4 |
| TxINT 5 | 14 | 3 | VI5 | 14 | 3 | RxINT 5 |
| TxINT 6 | 15 | 2 | VI6 | 15 | 2 | RxINT 6 |
| TxINT 7 | 16 | 1 | VI7 | 16 | 1 | RxINT 7 |

Read the pins carefully, because the two sockets are mirrored:

- **J15:** TxINT 0-7 are pins 9-16. VI0-VI7 are pins 8-1.
- **J16:** VI0-VI7 are pins 9-16. RxINT 0-7 are pins 8-1.

A straight shunt joins pin *n* to pin 17 − *n*, so in both sockets source *k* goes to VI*k*.

EXAMPLE: If you want to generate an interrupt on vectored interrupt line VI3 when data becomes
available from relative user 6, solder a wire between pins 2 and 12 of J16.

EXAMPLE: If you want to generate an interrupt on vectored interrupt line VI6 when data becomes
available from relative users 0,1,2, and 7, solder a wire to connect pins 1,6,7,8 and 15 of J16.

EXAMPLE: If you want to generate an interrupt on vectored interrupt line VI0 when relative user 2
is ready to accept a character, solder a wire to connect pins 8 and 11 of J15.

The three examples agree with the mirrored pin-out: in J16, RxINT 6 is pin 2 and VI3 is pin
12; RxINT 0, 1, 2 and 7 are pins 8, 7, 6 and 1, and VI6 is pin 15. In J15, VI0 is pin 8 and
TxINT 2 is pin 11.

## Channel 6/7 Interrupt Option (page 6)

Relative channels 6 and 7 are capable of generating a third interrupt called TxEMT/DSCHG\*. This
interrupt occurs when the transmitter has completed serialization of the last character loaded
or a change has occurred in the state of the DSR or DCD RS-232 status lines. Additional
information on this line may be found in the 2651 data sheet in this manual.

The TxEMT/DSCHG\* output from the 2651 may be jumpered to generate either a transmit or receive
interrupt. Due to the wire-OR capability of the interrupt outputs from the 2651, when jumpered,
the transmit interrupt will become TxRDY OR TxEMT/DSCHG or the receive interrupt will become
RxRDY OR TxRDY/DSCHG [sic]. Therefore, when jumpered, you must check the status register to
determine what condition caused the interrupt. (page 7)

**[sic]** "RxRDY OR TxRDY/DSCHG" means RxRDY OR TxEMT/DSCHG.

The following table will demonstrate where to install the shorting plug to generate the
appropriate interrupt.

| Channel number | TxEMT/DSCHG interrupt on the TxRDY line | on the RxRDY line |
|---|---|---|
| 6 | INSTALL J14 | INSTALL J13 |
| 7 | INSTALL J4 | INSTALL J3 |

## Interrupt Control Registers (page 7)

Two registers are provided for individually masking the transmit and receive interrupts from the
bus. On power-up or reset, all interrupts are disabled on the INTERFACER 3. To gain access to
these registers, a user channel must be enabled on the particular board to be altered. (You
cannot alter any interrupt register on the board set for users 0 thru 7 unless you have selected
one of those eight users) To enable a particular Transmit or Receive interrupt, a "1" must be
sent to the proper bit of the register. The registers are configured so that Data Bit 0 will
mask relative user 0, D1 will mask relative user 1, and so on with D7 masking relative user 7.
This is true for both the Transmit Interrupt Control Register (relative port 4) and the Receive
Interrupt Control Register (relative port 5).

EXAMPLE: To enable all Transmit interrupts on a particular INTERFACER 3, send a 0FFH to relative
port 4.

EXAMPLE: To enable the transmit interrupt on relative users 1, 4 and 6, send a 52H to relative
port 4.

EXAMPLE: To disable all Receive interrupts on a particular INTERFACER 3, send a 00H to relative
port 5.

EXAMPLE: To enable the Receive interrupt on relative users 2, 3 and 7, send a 8CH to relative
port 5.

## Interrupt Status Registers (pages 7-8)

Two registers are provided for checking the status of pending transmit and receive interrupts.
To gain access to these registers, a user channel must be enabled on the particular board to be
altered. (You cannot read any interrupt register on the board set for users 0 thru 7 unless you
have selected one of those eight users) If a Transmit or Receive interrupt is pending, a "1" will
be present in the proper bit of the status register. The registers are configured so that Data
Bit 0 contains the status of relative user 0, D1 contains the status of relative user 1, and so
on with D7 containing the status of relative user 7. This is true for both the Transmit
Interrupt Status Register (relative port 4) and the Receive Interrupt Status Register (relative
port 5). Remember, these status registers are read only! Writing into these registers will alter
the Interrupt Control Mask. In addition, the status of a channel's interrupts are available even
if those interrupts are masked "OFF". The Interrupt Control Register does not affect the reading
of the status from a register.

EXAMPLE: If all Transmit interrupts on a particular INTERFACER 3 are asserted, you will read a
0FFH at relative port 4.

EXAMPLE: If transmit interrupts are pending on relative users 1, 4 and 6, you will read a 52H
from relative port 4.

EXAMPLE: If there are no Receive interrupts pending on a particular INTERFACER 3 (no data
available), you will read a 00H from relative port 5.

EXAMPLE: If Receive interrupts are pending on relative users 2, 3 and 7, you will read a 8CH from
relative port 5.

Conversion note: the status registers show TxRDY and RxRDY of all eight USARTs, whatever the
mask (see "Interrupt Control/Status Logic", page 17). The mask only gates the VI outputs.

## USART Initialization (pages 8-9)

The serial channels on the INTERFACER 3 are implemented with a 2651 type USART from either
National Semiconductor or Signetics. Several of the USART parameters and channel control
functions are programmed by writing into or reading from certain registers in the 2651. They
are:

1. The baud rate.
2. The word length.
3. Whether or not a parity bit is generated.
4. Whether the parity is even or odd (if generated).
5. The number of stop bits.
6. Enabling and disabling the transmitter and receiver.
7. Setting and testing the RS-232 handshake lines.
8. Synchronous or asynchronous operation.

In addition, the normal status indication and data transfer functions are also handled through
the USART's registers.

A table of the various registers and where they appear in the I/O port map is shown in a previous
section and in the following tables.

"READ" or "INPUT" Ports

| Relative port address | UART register function |
|---|---|
| 00 hex | Data Port, read received data. |
| 01 hex | Status Port, read UART status info. |
| 02 hex | Mode Registers, read current UART mode. |
| 03 hex | Command Register, read current command. |

"WRITE" or "OUTPUT" Ports (page 9)

| Relative port address | UART register function |
|---|---|
| 00 hex | Data port, write transmit data. |
| 01 hex | SYN1/SYN2/DLE register, write sync bytes. |
| 02 hex | Mode registers, write mode bytes. |
| 03 hex | Command register, write command byte. |

### USART Initialization Sequence (page 9)

When bringing up the USART in asynchronous mode, the following sequence of events must occur:

1. Set Mode Register 1
2. Set Mode Register 2
3. Set Command Register
4. Begin normal USART operation

When bringing up the USART in transparent synchronous mode, all of the following sequence of
events must occur. If bringing up the USART in non-transparent synchronous mode, step 5 may be
omitted.

1. Set Mode Register 1
2. Set Mode Register 2
3. Set SYN1 Register
4. Set SYN2 Register
5. Set DLE Register
6. Set Command Register
7. Begin normal USART operation

## Data Registers (page 9)

The UART data registers are straight-forward in their operation. You write a byte to the data
register when you want to transmit that byte to an external serial device and you read the byte
in the data register to receive a byte from an external serial device. The UART will
automatically add the proper start and stop bits when transmitting and will remove them when
receiving.

## Status Register (pages 9-10)

The status register is used to determine the current state of the UART. Each bit of the status
register has a different meaning depending on whether it is high or low. (High means a logic one
or high level and low means a logic zero or low level.) The following table describes the
meaning of the status bits:

STATUS REGISTER FORMAT TABLE (page 10; read from the page image at 300 dpi)

| Bit | Name | Meaning |
|---|---|---|
| SR-7 | DATA SET READY | 0 = DSR\* input is high; 1 = DSR\* input is low |
| SR-6 | DATA CARRIER DETECT | 0 = DCD input is high; 1 = DCD input is low |
| SR-5 | FE/SYN DETECT | Async: 0 = normal, 1 = framing error. Sync: 0 = normal, 1 = SYN character detected |
| SR-4 | OVERRUN | 0 = normal; 1 = overrun error |
| SR-3 | PE/DLE DETECT | Async: 0 = normal, 1 = parity error. Sync: 0 = normal, 1 = parity error or DLE character received |
| SR-2 | TxEMT/DSCHG | 0 = normal; 1 = change in DSR\* or DCD\*, or transmit shift register is empty |
| SR-1 | RxRDY | 0 = receive holding register empty; 1 = receive holding register has data |
| SR-0 | TxRDY | 0 = transmit holding register busy; 1 = transmit holding register empty |

NOTE 1 (printed under the status table, but it belongs to Mode Register 1): BAUD RATE FACTOR IN
ASYNCHRONOUS MODE APPLIES ONLY IF EXTERNAL CLOCK IS SELECTED. FACTOR IS 16x IF INTERNAL CLOCK IS
SELECTED.

## Mode Registers (pages 10-11)

When bringing up the UART, its two mode registers must be set with various bit patterns that will
determine the operating modes. There are two registers, however they occupy only one I/O port
address. This is accomplished with internal sequencing logic that allows you to write the first
register (Mode Register 1) and then the second register (Mode Register 2). It is important to
write to Mode Register 1 first.

The meanings of the various bits in the mode registers are described on the next page.

MODE REGISTER 1 FORMAT (page 11)

| Bits | Meaning |
|---|---|
| MR1-7, MR1-6 | Sync: MR1-7 = number of SYN characters (0 = double SYN, 1 = single SYN); MR1-6 = transparency control (0 = normal, 1 = transparent). Async: stop bit length, 00 = invalid, 01 = 1 stop bit, 10 = 1½ stop bits, 11 = 2 stop bits |
| MR1-5 | Parity type: 0 = odd, 1 = even |
| MR1-4 | Parity control: 0 = disabled, 1 = enabled |
| MR1-3, MR1-2 | Character length: 00 = 5 bits, 01 = 6 bits, 10 = 7 bits, 11 = 8 bits |
| MR1-1, MR1-0 | Mode and baud rate factor (note 1): 00 = synchronous 1x rate, 01 = asynchronous 1x rate, 10 = asynchronous 16x rate, 11 = asynchronous 64x rate |

MODE REGISTER 2 FORMAT

| Bits | Meaning |
|---|---|
| MR2-7, MR2-6 | Not used (the second box is printed "MR1-6" **[sic]**) |
| MR2-5 | Transmitter clock: 0 = external, 1 = internal |
| MR2-4 | Receiver clock: 0 = external, 1 = internal |
| MR2-3 to MR2-0 | Baud rate selection (below) |

| MR2-3..0 | Baud | MR2-3..0 | Baud | MR2-3..0 | Baud |
|---|---|---|---|---|---|
| 0000 | 50 | 0110 | 600 | 1100 | 4800 |
| 0001 | 75 | 0111 | 1200 | 1101 | 7200 |
| 0010 | 110 | 1000 | 1800 | 1110 | 9600 |
| 0011 | 134.5 | 1001 | 2000 | 1111 | 19200 |
| 0100 | 150 | 1010 | 2400 | | |
| 0101 | 300 | 1011 | 3600 | | |

This completes the description of the Mode Registers. Remember that you must always write both
mode registers, with Mode Register 1 first.

## Command Register (page 11)

The Command Register is used to set the operating mode (sync or async), enable or disable the
receiver and/or transmitter, force a "break" condition, reset the error flags and control the
state of the RTS and DTR outputs.

COMMAND REGISTER FORMAT

| Bits | Meaning |
|---|---|
| CR-7, CR-6 | Operating mode: 00 = normal operation; 01 = async: automatic echo mode, sync: SYN and/or DLE stripping mode; 10 = local loop back; 11 = remote loop back |
| CR-5 | Request to send: 0 = forces RTS\* output high; 1 = forces RTS\* output low |
| CR-4 | Reset error: 0 = normal; 1 = reset error flag in status register (FE, OE, PE/DLE detect) |
| CR-3 | Async: force break, 0 = normal, 1 = force break. Sync: send DLE, 0 = normal, 1 = send DLE |
| CR-2 | Receive control (RxEN): 0 = disable, 1 = enable |
| CR-1 | Data terminal ready: 0 = forces DTR\* output high; 1 = forces DTR\* output low |
| CR-0 | Transmit control: 0 = disable, 1 = enable |

## Serial Mode Jumpers (page 12)

The INTERFACER 3 board with its serial programming jumpers allows the user to adapt relative
channels 6 and 7 to all standard RS-232 pin configurations. In RS-232 mode, these jumpers may be
set so that this board operates in a "master" mode where it behaves as the Data Terminal
Equipment (DTE), or it may be set so that the board operates in a "slave" mode where it behaves
as the Data Communication Equipment (DCE). Since almost all CRT terminals and serial interface
printers operate as the "master" or as the Data Terminal Equipment, then the INTERFACER 3 board
must operate as the "slave" or Data Communication Equipment. (For this reason, relative channels
0 - 5 are set to operate in this mode.) For example, to connect the INTERFACER 3 to a terminal
like an Televideo or a Hazeltine, relative channels 0 - 5 will connect directly and relative
channels 6 and 7 require that serial mode jumpers (J1 and J2) should be set in "slave" mode as
shown on the following table. To connect relative channels 6 and 7 to a modem is a different
set-up because modems are set to operate as "slaves". When connected to a modem, the serial mode
jumpers (J1 and J2) of the INTERFACER 3 should be set in the "master" mode as shown on the
following table.

PROGRAMMING JUMPERS (drawings):

- SLAVE MODE, J1/J2: for connections to CRT terminals, printers, etc. The drawing shows a 16-pin
  socket (pin 1 at the top left) with straight connections across: the top seven pin pairs are
  joined, and the bottom pair is open.
- MASTER MODE, J1/J2: for connection to MODEMS. The drawing shows crossed connections, in pairs,
  in the upper part of the socket.

Conversion note: the drawings are too small to read pin by pin. Use the PDF and the logic diagram
for the exact wiring. The pin swap they make is the one in the starred notes of "RS-232C Control
Lines" (DTR 20/6, RTS 4/5, DSR 6/20, CTS 5/4).

## RS-232C Control Lines (page 13)

The RS-232 control and data lines are defined as shown below. The EIA RS-232 standard defines a
signal line at greater than +3V (+12V typical) to be "SPACING" and a signal line at less than -3V
(-12V typical) to be "MARKING".

| Pin | Circuit | Dir. | Name | Description |
|---|---|---|---|---|
| 1 | AA | | | PROTECTIVE GROUND |
| 2 | BA | TO DCE | TxD | TRANSMITTED DATA |
| 3 | BB | TO DTE | RxD | RECEIVED DATA |
| 4 | CA | TO DCE | RTS | REQUEST TO SEND |
| 5 | CB | TO DTE | CTS | CLEAR TO SEND |
| 6 | CC | TO DTE | DSR | DATA SET READY |
| 7 | AB | | | SIGNAL GROUND |
| 8 | CF | TO DTE | DCD | REC'D LINE SIGNAL DET. |
| 15 | DB | DCE SOURCE | TSET | TRANS. SIG. ELE. TIMING |
| 17 | DD | DCE SOURCE | RSET | REC'D SIG. ELE. TIMING |
| 20 | CD | TO DCE | DTR | DATA TERMINAL READY |

Five hardwired RS-232 handshaking signals are provided for interfacing to equipment needing these
lines as shown below. Output lines may be set either "MARKING" or "SPACING" and their state may
be altered by software commands as described in the USART INITIALIZATION Section under Command
Register.

OUTPUT LINES

| Name | RS-232 line | DB25 pin connection |
|---|---|---|
| DTR | CD | 20 OR 6\* |
| RTS | CA | 4 OR 5\* |

INPUT LINES

| Name | RS-232 line | DB25 pin connection |
|---|---|---|
| DSR | CC | 6 OR 20\* |
| CTS | CB | 5 OR 4\* |
| DCD | CF | 8 |

\*NOTE: Non-starred pin numbers indicate the DB25 pin number for relative channels 0 - 5 and when
the Serial Mode Jumpers of relative channels 6 and 7 are set for "master" mode. The starred pin
numbers indicate the DB25 pin number on relative channels 6 and 7 when the Serial Mode Jumpers
are set for "slave" mode.

**[sic]** The note has channels 0-5 backwards. The logic diagram (page 32, "6 REMAINING
CHANNELS") wires them as DCE, the "slave" of page 12: TxD out on pin 3, RTS out on 5, DTR out on
6; RxD in from 2, CTS in from 4, DSR in from 20, DCD in from 8. Those are the starred numbers.
The non-starred numbers are channels 6 and 7 in "master" mode only.

## Synchronous Mode Clock Driver/Receivers (page 14)

Relative channels 6 and 7 can either transmit or receive the synchronous signal timing element
signals. The typical configuration requires that the DATA COMMUNICATION EQUIPMENT (DCE) be the
source of the of [sic] the synchronous transmit and receive clocks. The INTERFACER 3 is capable
of independently transmitting or receiving either of the clocks in either DCE or DTE modes. The
following table will describe how the pin shunts should be set for transmitting or receiving the
clocks.

| Channel | Receive sync clock: transmit | Receive sync clock: receive | Transmit sync clock: transmit | Transmit sync clock: receive |
|---|---|---|---|---|
| 6 | INSTALL J11 | INSTALL J12 | INSTALL J9 | INSTALL J10 |
| 7 | INSTALL J7 | INSTALL J8 | INSTALL J5 | INSTALL J6 |

EXAMPLE: If you want relative channel 7 to transmit both its transmit and receive sync clocks,
install pin shunts on J7 and J5.

EXAMPLE: If you want relative channel 6 to receive both its transmit and receive sync clocks,
install pin shunts on J10 and J12.

## Theory of Operation (pages 15-17)

The INTERFACER 3 can be roughly divided into seven subsections for describing its operation.
These sections include: The S-100 Bus Drivers, the I/O Port Decode Logic, the Strobe Generation
Logic, the Wait State Logic, the Interrupt Control/Status Logic, the USART, and the RS-232 Level
Conversion Logic.

### S-100 Bus Drivers (page 15)

The separate data input and output data buses of the S-100 bus are converted to a bi-directional
data bus by octal drivers U44 and U58. Data from the S-100 bus is driven onto the internal data
bus by U58 only when sOUT goes high, indicating an output operation. The internal data bus is
driven onto the S-100 bus when DOEN\* goes low, indicating that a valid board select (SEL) and
pDBIN are high (NAND-U45).

All S-100 bus signals are buffered onto the board if the line would otherwise have more than 1
LSTTL load. Address lines A0, A1, A2, and pDBIN are buffered onto the board by 2/3 of hex buffer
U48, and the lines sOUT, sINP, pWR\*, O, and pSTVAL\* are inverted using portions of U29, U43, and
U50.

### I/O Port Decode Logic (pages 15-16)

The eight port block that the INTERFACER 3 occupies is decoded by six open collector X-OR gates
(U46 and U47). Five of these gates decode address lines A3-A7 by comparing against positions 2-6
of switch S1, and the last section compares sOUT and sINP\* to determine if an I/O operation is
occurring. When all compare conditions are satisfied, ASEL goes high. Closing position 1 of S1
will ground ASEL and disable the board completely.

A valid board select (SEL\*) is generated (by 1/3 of U32), when ASEL goes high along with USEL
(indicating that this boards select number is active) and A1 and A2 are not both high
(indicating the USER SELECT PORT is not selected). SEL\* is disabled by 1/3 of U32 when the USER
SELECT PORT is enabled so that conflicts between up to four boards does not occur.

A USER SELECT write occurs when ASEL, A1, A2, sOUT, and STROBE go high. This generates OUT0\*
(U32) which clocks the least significant five bits on the bus (D0-D4) into hex latch U34. The
three low order bits of U34 are decoded into eight chip enables (CE0\* - CE7\*) by U35 when SEL is
high and A2 and ESTROBE\* are low. The two high order bits of U34 are compared to switch
positions 7 and 8 of S1 by 1/2 of U47 (X-NOR) to decode a current user board select signal USEL.
Access to registers on the board requires that USEL be high before access is gained.

The four interrupt read and write strobes are generated by decoder U49 when A2 is high and SEL\*
and STROBE\* are low. A0, A1, and sINP\* determine which output becomes active at the proper time.
(page 16)

Conversion notes (interpretation of the decode text):

- The user select port is decoded on A1 = A2 = 1, so relative ports 6 and 7 both write it. The
  text names only port 7; whether A0 is decoded for it is not stated.
- The user select latch U34 is written whatever its board select bits are, so every Interfacer 3
  at the same base address latches every user select write. Each board then answers only when
  its own S1-7/S1-8 match BS0/BS1 (USEL). This is how up to four boards share one 8-port block.
- An IN from the user select port (A1 = A2 = 1) is not decoded, since SEL\* needs A1 and A2 not
  both high.

### Strobe Generation Logic (page 16)

In order to gain additional access time in an I/O cycle for the 2651 USARTs, the INTERFACER 3
generates early strobes based on valid status. S-100 bus strobes pDBIN and pWR\* are gated
together (U30) and inverted to generate STROBE and STROBE\*. These signals indicate that a bus
strobe is occurring. The interrupt registers and user select port have their data gated by
STROBE because they are TTL and capable of very high speed operation. Since the 2651 type USART
is a MOS device and has an access time of approximately 250 nS, an early strobe is generated so
that wait states are avoided whenever possible. A status valid signal, ESTATVAL\*, is generated
whenever pSYNC is high and pSTVAL\* is low. ESTATVAL\* clears "D" flop U33a to generate ESTROBE\*,
which becomes one term of the USART chip enable decoder U35. The termination of STROBE\* causes a
"1" to be clocked into U33a and terminate ESTROBE\*.

### Wait State Logic (page 16)

To allow operation with high speed processors, a wait state generator allows the addition of one
or two wait cycles. U31 forms a two bit shift register clocked by O\*. A wait state is left
pending after STROBE goes low, and when STALL1\* or STALL2\* and A2 are low (U30), and SEL is high
(U45), WAIT\* is generated. STALL1\* is clocked out on the next rising edge of O\* after STROBE goes
high, and STALL2\* is clocked out the following cycle. The pRDY\* line is pulled low by U48 when
WAIT\* goes low. When neither STALL1\* or STALL2\* is connected on J17, no wait states will be
generated.

### Interrupt Control/Status Logic (pages 16-17)

The interrupt logic consists of two 8-bit latches for enabling interrupts onto the bus, two 8-bit
buffers for reading current interrupt status, and sixteen 2-input open collector NAND buffers for
driving the interrupts on the bus.

Two 8-bit latches are formed by four 4-bit latches (U38, U41, U52, and U55) for generating the
interrupt enable mask. The Q outputs become the RxINTENx and TxINTENx interrupt enables for
selectively masking "OFF" individual interrupts. Upon power-up or reset, these latches are
cleared by CLR\* so that all interrupts are disabled.

The TxRDY and RxRDY interrupt outputs from the 2651 USARTs are inverted to form active high
interrupt signals. These interrupt signals are fed to one input of the open collector NAND
buffer, with the corresponding interrupt enable fed to the other input. The resulting interrupt
outputs (TxINTx and RxINTx) are capable of driving the VI0-7 lines directly, and are brought out
to J15 and J16 for jumpering to the appropriate line.

Two 8-bit buffers are formed from four quad tri-state buffers (U37, U42, U51, and U56) for gating
the current USART interrupts (TxRDYx and RxRDYx) onto the bus as status information. Since the
buffers use Tx and Rx RDY instead of Tx and Rx INT lines, the status of disabled interrupts are
displayed as well as enabled interrupts.

Relative channels 6 and 7 allow jumpering the TxEMT/DSCHG interrupt from the USART to either the
TxRDY or RxRDY interrupt outputs. This is possible since the outputs from the 2651 are open drain
and may be wire-ORed.

### USARTs (page 17)

The 2651 type USART is quite sophisticated in that it can run in both asynchronous as well as
synchronous modes. In addition, the part has an internal baud rate generator, RS-232 status and
control bits, up to three interrupt outputs, and the capability of transmitting as well as
receiving baud clocks.

The chip enable (CE) and read/write (R\*/W) lines are operated by initially determining whether
a read or a write will occur (sINP\* to R\*/W) and then strobing the part with CE\*. Address lines
A0 and A1 determine which of four registers will be selected and CLR resets the USART.

The baud rate clock BAUDCLK is generated by a 5.0688 MHz crystal oscillator formed from three
inverters (U29) and crystal X1.

### RS-232 Level Conversion Logic (page 17)

Each USART has a full compliment [sic] of RS-232 handshaking lines for devices that require them.
Industry standard 1488 and 1489 receivers and transmitters are used throughout for highest
performance. In addition to the data lines TxD and RxD, each channel has a RTS and DTR output and
a CTS, DSR, and DCD input. All three RS-232 status lines have pullup resistors to +12V so that
floating inputs are pulled high.

Relative channels 0 - 5 have the RS-232 lines set for direct connection to CRT terminals and
printers. Relative channels 6 and 7 may be set for both DCE and DTE modes by wiring new jumpers
for J1 and J2.

Relative channels 6 and 7 are capable of sending and receiving both the transmit and receive
baud [sic] clocks for running in synchronous mode. An RS-232 driver and a receiver are provided
for RxC and TxC, and either one may be jumpered in.

Conversion note: "pulled high" to +12V is SPACING (ON) on the RS-232 side. Through the 1489
(an inverter) the 2651 sees DSR\*/DCD\* low, so an unconnected input reads 1 in SR-7 and SR-6
(asserted). This is the opposite of the Interfacer 1, where an open 1489 input reads OFF. The
status register has no CTS bit, although page 13 lists CTS as an input. What the 2651 does with
CTS\* is in the data sheet (pages 25-30).

## User Notes (page 18)

The page has the heading "USER NOTES" and is otherwise blank.

## Sample Test Program for Running in Asynchronous Mode (pages 19-20)

Transcribed as printed.

```
*
*
*               INTERFACER 3 TEST PROGRAM
*
* This program will initialize all 2651s for asynchronous operation
at 9600 baud with 8 data bits, one stop bit, no parity. This program
will echo all characters received on any user channel (from 0 to 31)
and if any user sends a ^C, the program will terminate and return back
to CP/M.

NOTE: This program assumes that the console device is either an
INTERFACER 1 or 2 addressed at ports 0 and 1.
*
*
base    equ     18h
udata   equ     BASE+0h ;data port in and out
ustat   equ     BASE+1h ;status register port
mode    equ     BASE+2h ;mode register port
commr   equ     BASE+3h ;command register port
txreg   equ     BASE+4h ;tx int register
rxreg   equ     BASE+5h ;rx int register
user    equ     BASE+7h ;port to select user
exit    equ     0       ;CP/M reentry point
tbmt    equ     01h     ;transmitter buffer empty
dav     equ     02h     ;data available
*
*
*
        org     100h
Start   mvi     a,0ffh  ;init user
Loop    inr     a       ;next user
        cpi     20H     ;check for final uart
        jz      echo    ;start echo routine
        out     user    ;select uart
        mov     b,a     ;save user in b
        call    init    ;init the uart
        mov     a,b     ;restore user
        jmp     loop    ;next
Init    mvi     a,0CEh  ;set up the 2651
        out     mode    ;send to mode register 1
        mvi     a,7Eh   ;9600 baud, internal clocks
        out     mode    ;SEND BYTE TO M.R. 2
        mvi     a,27h   ;could be 07h (no 1420)
        out     commr
        ret
Echo    mvi     a,0FFh  ;mask value
        out     txreg   ;set tx int reg
        out     txreg   ;set rx int reg
Loop1   inr     a       ;next user
        out     user    ;select uart
        mov     b,a     ;save user in b
        call    cstat   ;check for data
        cpi     0AAh    ;data if aa
        cz      ok      ;do echo loop
        mov     a,b     ;restore user
        jmp     loop1   ;next
Ok      call    inloop  ;get data
        call    oloop   ;output data
        ret
Cstat   in      ustat   ;look for key entry
        ani     dav     ;check status
        jz      nodat   ;no data
        mvi     a,0AAh  ;data char
        ret
Nodat   mvi     a,0     ;no data char
        ret
Inloop  in      ustat   ;look for key entry
        ani     dav     ;check the status
        jz      inloop  ;wait for key entry
        in      udata   ;get key entry
        ani     7Fh     ;mask parity off
        cpi     03h     ;has a ^c been hit?
        jz      done    ;return to CP/M
        mov     e,a     ;save input in E reg.
        ret
Oloop   in      ustat   ;check ready for output
        ani     tbmt    ;check status
        jz      oloop   ;wait for ready
        mov     a,e     ;get data
        out     udata   ;output character
        ret
Done    jmp     exit    ;return to cp/m
        end
```

**[sic]** notes on the listing:

- `Echo` writes `txreg` twice; the comment on the second says "set rx int reg". It should be
  `out rxreg`.
- The `Echo` loop increments the user number from FF without a limit, so it walks 00-FF; only
  D0-D4 are latched, so it cycles through users 0-31.
- 0CEh in Mode Register 1 is 2 stop bits (MR1-7,6 = 11), not the "one stop bit" of the header.

## Sample Test Program for Synchronous Mode (pages 21-22)

Transcribed as printed.

```
*
*
*       INTERFACER 3 SYNCHRONOUS TEST PROGRAM
*
*
* This program will take characters typed on the console and transmit
them synchronously at 19.2K baud out of relative user 6 to relative
user 7, and then back out of 7 to 6 and back to the console. When a
control C (^C) is hit on the console, the program will terminate and
re-enter CP/M.

NOTE: This program assumes that the console device is an INTERFACER 1
or 2 at ports 0 and 1. It does not use direct BIOS entry points. The
synchronous clock jumpers should be set as described in the example in
the SYNCHRONOUS MODE CLOCK DRIVER/RECEIVER section. The
SERIAL MODE JUMPERS should be set so that channel 7 is in master mode
and 6 is in slave mode.
*
*
base    equ     10h
udata   equ     base+0h ;data port in and out
ustat   equ     base+1h ;status register port
mode    equ     base+2h ;mode register port
commr   equ     base+3h ;command register port
txreg   equ     base+4h ;tx int register
rxreg   equ     base+5h ;rx int register
user    equ     base+7h ;port to select user
exit    equ     0       ;CP/M reentry point
cstat   equ     01h     ;console status port
cdata   equ     00h     ;console data port
tbmt    equ     01h     ;transmitter buffer empty
dav     equ     02h     ;data available
*
*
*
        org     100h
START   mvi     a,0ffh  ;mask value
        out     txreg   ;set tx int reg
        out     rxreg   ;set rx int reg
INIT6   mvi     a,6h    ;init user 6
        out     user    ;select uart
        mvi     a,08cH  ;set up the 2651
        out     mode    ;send to mode register 1
        mvi     a,0fh   ;19200 baud, external clocks
        out     mode    ;send to mode register 2
        mvi     a,0a5h  ;synch character
        out     ustat   ;send to synch reg
        mvi     a,67h   ;synch strip mode
        out     commr   ;send to command register
        mvi     a,0a5h  ;dummy synch character
        out     udata   ;poke in butt to start
INIT7   mvi     a,7h    ;init user 7
        out     user    ;select uart
        mvi     a,08cH  ;set up the 2651
        out     mode    ;send to mode register 1
        mvi     a,3fh   ;19200 baud, internal clocks
        out     mode    ;send to mode register 2
        mvi     a,0a5h  ;SYN1 character
        out     ustat   ;send to synch reg
        mvi     a,67h   ;synch strip mode
        out     commr   ;send to command register
        mvi     a,0a5h  ;dummy synch character
        out     udata   ;poke in butt
CONIN   in      cstat   ;look for key entry
        ani     dav     ;check status
        jz      conin   ;no data
        in      cdata   ;get char
        ani     7fh     ;mask parity off
        cpi     03      ;has a ^c been hit?
        jz      done    ;return to CP/M
        mov     l,a     ;save in l
OUT6    mvi     a,6     ;user 6
        out     user    ;select
OUT6L   in      ustat   ;look for ready
        ani     tbmt    ;check the status
        jz      out6l   ;wait for ready
        mov     a,l     ;restore character
        out     udata   ;output char
IN7     mvi     a,7     ;user 7
        out     user    ;select
IN7L    in      ustat   ;get status
        ani     dav     ;check for char
        jz      in7l    ;no char
        in      udata   ;get char
        mov     l,a     ;save char
OUT7    in      ustat   ;check ready for output
        ani     tbmt    ;check status
        jz      out7    ;wait for ready
        mov     a,l     ;get data
        out     udata   ;output character
IN6     mvi     a,6     ;user 6
        out     user    ;select
IN6L    in      ustat   ;get status
        ani     dav     ;check for char
        jz      in6l    ;no char
        in      udata   ;get char
        mov     l,a     ;save char
CONOUT  in      cstat   ;check ready for output
        ani     tbmt    ;check status
        jz      conout  ;wait for ready
        mov     a,l     ;get data
        out     cdata   ;output character
        jmp     conin
DONE    jmp     exit    ;return to cp/m
        end
```

## Sample Program for Using the Interfacer 3 as the CP/M Console (pages 23-24)

Transcribed as printed.

```
;       CompuPro INTERFACER 3 equates.

GBI3:   EQU     10h             ;INTERFACER 3 BASE
GBUD:   EQU     GBI3+0          ;Uart data port
GBUS:   EQU     GBI3+1          ;Uart status port
GBUM:   EQU     GBI3+2          ;Uart mode port
GBUC:   EQU     GBI3+3          ;Uart command port
GBUSR   EQU     GBI3+7          ;User select register
I3DAV:  EQU     02H             ;INTERFACER 3 DAV
I3TBMT: EQU     01H             ;INTERFACER 3 TBMT

;       C O N S O L E   I N I T I A L I Z A T I O N
;
;       This routine performs the initialization
;       required by the INTERFACER 3 USART.
;
sTINIT  MVI     A,0             ;select user "0"
        OUT     GBUSR           ;output to select user
        MVI     A,0EEH          ;8 bits, no parity, 2 stops
        OUT     GBUM            ;Set up mode register 1
        MVI     A,07EH          ;9600 baud
        OUT     GBUM            ;Set up mode register 2
        MVI     A,027H          ;dtr low, no break,
                                ;no reset, rts low
        OUT     GBUC            ;Set up command port
        RET

;       C O N S O L E   S T A T U S
;
;       This routine samples the Console status and returns
;       the following values in the A register.
;
;       EXIT    A = 0 (zero), means no character
;               currently ready to read.
;
;               A = FFh (255), means character
;               currently ready to read.

sCONST  IN      GBUS            ;Input from port
        ANI     I3DAV           ;Mask data available
        RZ                      ;If data not available
        ORI     0FFH
        RET

;       C O N S O L E   I N P U T
;
;       Read the next character into the A register,
;       clearing the high order bit. If no character
;       currently ready to read then wait for a character
;       to arrive before returning.
;
;       EXIT    A = character read from terminal.

sCONIN  IN      GBUS            ;Get status from uart
        ANI     I3DAV
        JZ      sCONIN
        IN      GBUD
        ANI     7Fh
        RET

;       C O N S O L E   O U T P U T
;
;       Send a character to the console. If the console
;       is not ready to receive a character wait until
;       the console is ready.
;
;       ENTRY   C = ASCII character to output to console.

sCONOUT IN      GBUS            ;Get uart status
        ANI     I3TBMT          ;Test if buffer empty
        JZ      sCONOUT
        MOV     A,C
        OUT     GBUD
        RET

;       End     GBcbioI3.asm
```

Conversion note: 0EEH in Mode Register 1 is 2 stop bits, parity disabled (MR1-4 = 0; MR1-5 = 1
is ignored), 8 bits, async 16x. It agrees with the comment.

## INS2651 Programmable Communications Interface (pages 25-30)

The National Semiconductor INS2651 data sheet, dated October 1980, printed sideways. It is not
transcribed in full; use the PDF. The pages hold:

| Page | Contents |
|---|---|
| 25 | General description, features, system configuration diagram; absolute maximum ratings, DC characteristics, capacitance |
| 26 | AC characteristics (bus parameters, other timings); timing waveforms (reset, read/write, clock, transmit, receive) |
| 27 | Timing waveforms, continued (synchronous and asynchronous mode); block diagram; functional pin definitions, input signals |
| 28 | Output signals; input/output signals; pin configuration; programming (Table 1, register addressing); Figure 1, initialization flowchart |
| 29 | Mode Register 1 and 2, Command and Status register formats (the same tables as pages 10-11); Table 2, baud rate generator characteristics |
| 30 | Operation (general, synchronous mode, asynchronous mode); physical dimensions; sales offices |

Facts from the data sheet that matter for this board (read from the page images at 300 dpi):

- **DSR\*, pin 22:** a general-purpose input which, when low, indicates either the Data Set Ready
  or Ring condition. Its complement is stored as Status Register bit 7. A change in state of
  this input causes a low output on TxEMT/DSCHG\*.
- **DCD\*, pin 16:** when low, enables the receiver to operate. The complement of this input is
  stored as Status Register bit 6, and an input change in state causes a low output on
  TxEMT/DSCHG\*.
- **CTS\*, pin 17:** when low, enables the transmitter to operate. When high, holds the TxD output
  in MARK condition.
- **TxRDY\*, pin 15:** open-drain. A low indicates that the Transmit Holding Register (THR) is
  ready to accept a data character from the CPU. It is the complement of Status Register bit 0,
  goes high when a data character is loaded, and is valid only when the transmitter is enabled.
- **RxRDY\*, pin 14:** open-drain. A low indicates that the Receive Holding Register (RHR) has a
  character ready for input to the CPU. It is the complement of Status Register bit 1, and goes
  high either when the Receive Holding Register is read by the CPU or when the receiver is
  disabled.
- **TxEMT/DSCHG\*, pin 18:** open-drain. A low indicates that either the transmitter has completed
  serialization of the last character loaded by the CPU, or that a change of state of the DSR\* or
  DCD\* inputs has occurred. If the TxEMT condition does not exist, this output goes high when the
  Status Register is read by the CPU. Otherwise, the Transmit Holding Register must be loaded by
  the CPU for this line to go high. It is the complement of Status Register bit SR2.
- **TxD, pin 19:** held in the marking state (logic 1) when the transmitter is disabled.
- **BRCLK, pin 20:** the 5.0688 MHz clock input to the internal baud rate generator.
- Table 2 gives the 16x clock divisor for each baud rate (crystal 5.0688 MHz). For example, 9600
  baud is divisor 33 (153.6 kHz, no error listed) and 19200 baud is divisor 16 (316.8 kHz against
  307.2 kHz theoretical, 3.125 % error).

Conversion note: with the +12V pull-ups on page 17, an unconnected DCD and CTS read low at the
2651, so the receiver and transmitter of an unconnected channel are enabled.

## Logic Diagram (pages 31-32) and USARTs 0-7 (pages 33-36)

Two sheets, "INTERFACER 3 174C ©1981, COMPUPRO division GODBOUT ELECTRONICS", page 1 of 2 and
page 2 of 2, then four pages with two USART channels each. They are described here, not redrawn;
use the PDF for the drawings. Read from the page images at 300 dpi.

Sheet 1 (page 31):

- **Data bus:** U58 (81LS95/97) drives DO0-DO7 onto the internal bus, enabled by sOUT\*. U44
  (74LS244) drives the internal bus onto DI0-DI7, enabled by DOEN\*.
- **Baud clock:** X1, 5.0688 MHz, with R7, R8 (1K), C9 (.01 uF) and three U29 inverters, gives
  BAUDCLK.
- **Address compare:** U46 and U47 (74LS266, open-collector X-NOR) compare A7-A3 with S1-2 to
  S1-6, and sOUT with sINP. S1-1 grounds the wired-OR line (ASEL) through U47 to disable the board.
- **User select latch:** U34 (74LS174) latches D0-D4 on OUT0\*, cleared by CLR\*. Q0-Q2 go to U35
  (74LS138) A-C. U35 G1 = SEL, G2A = A2, G2B = ESTROBE\*. Its outputs Y0-Y7 are CE0\*-CE7\*, the
  chip enables of USARTs 0-7. Q3-Q4 go to the two U47 X-NOR gates against S1-7 and S1-8; the
  outputs are wired together with SR4 to +5V as USEL.
- **Board select:** U32 gates ASEL, USEL and the "A1 and A2 not both high" term into SEL\* (and
  SEL through U36). Another U32 section, with A2, A1, sOUT and STROBE, gives OUT0\*.
- **Interrupt register strobes:** U49 (74LS138): A = A0, B = A1, C = sINP\*; G1 = A2,
  G2A = SEL\*, G2B = STROBE\*. Y0 = IN2\* (read relative port 4), Y1 = IN3\* (read port 5),
  Y4 = OUT2\* (write port 4), Y5 = OUT3\* (write port 5). Y2, Y3, Y6, Y7 are not used.
- **Bus buffers:** U48 (74LS367) buffers A2, A1, A0 and pDBIN. U45 gates pDBIN with SEL into
  DOEN\*. U30 and U50 make STROBE and STROBE\* from pDBIN and pWR\*. U43, U57 and U29 make sOUT,
  sOUT\* and sINP\*.
- **Clear:** U45 combines pRESET\* with SLVCLR\* (through J18 B-C) or POC\* (J18 A-C) into CLR and
  CLR\* (through U29).
- **Wait states:** U31 (74LS74, two flip-flops) clocked by Φ\* gives STALL1\* and STALL2\* to J17.
  U30 and U45 combine the J17 selection, A2 and SEL into WAIT\*, and U48 drives pRDY.
- **Early strobe:** U45 makes ESTATVAL\* from pSTVAL\* and pSYNC. It clears U33A, which gives
  ESTROBE\*. STROBE\* clocks U33A back.
- U33B is clocked by U34-4 with D from U34-5; its outputs are not connected (NC). Several spare
  gates are marked NC.

Sheet 2 (page 32):

- **Status buffers:** U42 and U37 (74LS125) put TxRDY0-TxRDY7 on D0-D7, enabled by IN2\*. U56 and
  U51 put RxRDY0-RxRDY7 on D0-D7, enabled by IN3\*.
- **Mask latches:** U41 and U38 (74LS175) latch D0-D7 on OUT2\* as TxINTEN0-7. U55 and U52 latch
  D0-D7 on OUT3\* as RxINTEN0-7. CLR\* clears all four.
- **One channel, drawn once:** the 2651/2661 USART, D0-D7, A0, A1, R/W\* from sINP\*, CE from CEx,
  RESET from CLR, BRCLK from BAUDCLK.
  - TxRDY (pin 15) and RxRDY (pin 14), each with a pull-up, are inverted to TxRDYx and RxRDYx.
    A 2-input NAND (TxRDYx, TxINTENx) gives TxINTx, and (RxRDYx, RxINTENx) gives RxINTx.
  - TxEMT/DSCHG (pin 18) goes to two jumper pads, to the TxRDY or the RxRDY line (channels 6 and
    7 only).
  - "2 CHANNELS ONLY" (6 and 7): the RS-232 drivers and receivers go through a 16-pin socket (J1
    or J2) to the DB-25 pins. Socket pins 1-7 are TxD, RxD, RTS, CTS, DSR, DTR, DCD on the board
    side; pins 16-10 go to DB-25 pins 3, 2, 5, 4, 20, 6, 8. Pin 8 goes to +12V and pin 9 to -12V
    through resistors. RxC (pin 25) and TxC (pin 9) each have a driver and a receiver to DB-25
    pins 17 and 15, selected by jumpers.
  - "6 REMAINING CHANNELS" (0-5): TxD → pin 3, RxD ← pin 2, RTS → pin 5, CTS ← pin 4,
    DSR ← pin 20, DTR → pin 6, DCD ← pin 8. CTS, DSR and DCD have pull-ups to +12V.
  - DB-25 pins 1 and 7 go to ground.
- **J15 and J16:** TxINT0-7 on J15 pins 9-16; VI0-VI7 on J15 pins 8-1 and J16 pins 9-16;
  RxINT0-7 on J16 pins 8-1. This agrees with the page 6 table.
- **Power:** -16V (bus pin 52) → U1 7912 → -12V. +16V (pin 2) → U2 7812 → +12V. +8V (pins 1, 51)
  → U59 and U60 (7805) → +5V. Ground on pins 50 and 100.

USARTs 0-7 (pages 33-36), the chips and gates for each channel:

| Channel | USART | CE | TxINT gate | RxINT gate | Notes |
|---|---|---|---|---|---|
| 0 | U28 | CE0\* | U40 | U54 | RS-232 through U19, U20 |
| 1 | U27 | CE1\* | U40 | U54 | U17, U18 |
| 2 | U26 | CE2\* | U40 | U54 | U15, U16 |
| 3 | U25 | CE3\* | U40 | U54 | U13, U14 |
| 4 | U24 | CE4\* | U39 | U53 | U11, U12 |
| 5 | U23 | CE5\* | U39 | U53 | U9, U10 |
| 6 | U22 | CE6\* | U39 | U53 | J2; J9-J12 clocks; J13/J14 TxEMT |
| 7 | U21 | CE7\* | U39 | U53 | J1; J5-J8 clocks; J3/J4 TxEMT |

On channels 0-5, TxEMT/DSCHG (pin 18) is not connected. The TxRDY/RxRDY inverters are U43 and U57
(channels 0-3) and U36 and U50 (channels 4-7; channel 6 uses two sections of U36).

**[sic]** The page 32 heading says "See pages 34 - 37 for pin numbers of each USART Channel". The
channel pages are 33-36.

## Parts List (page 38)

PDF page 41 (printed page 37) has only the heading "PARTS LIST & COMPONENT LAYOUT SECTION".

| Part | Value | Part | Value |
|---|---|---|---|
| U1 | 7912 | U48 | 74LS367 |
| U2 | 7812 | U49 | 74LS138 |
| U3 | 1488 | U50 | 74LS04 |
| U4 | 1489 | U51 | 74LS125 |
| U5 | 1488 | U52 | 74LS175 |
| U6 | 1489 | U53-U54 | 74LS38 |
| U7 | 1488 | U55 | 74LS175 |
| U8 | 1489 | U56 | 74LS125 |
| U9 | 1488 | U57 | 74LS04 |
| U10 | 1489 | U58 | 81LS95/97 |
| U11 | 1488 | U59-60 | 7805 |
| U12 | 1489 | | |
| U13 | 1488 | **Resistors** | |
| U14 | 1489 | R1-R5 | 4.7K |
| U15 | 1488 | R6 | 1.8K |
| U16 | 1489 | R7-R8 | 1.2K |
| U17 | 1488 | SR1-6 | 5.1K |
| U18 | 1489 | | |
| U19 | 1488 | **Capacitors** | |
| U20 | 1489 | C1-C4 | 2.7 uF |
| U21-U28 | 2651/61 | C5-C8 | 39 uF |
| U29 | 74LS04 | C9 | .01 uF |
| U30 | 74LS02 | (39) | .01 uF |
| U31 | 74LS74 | | |
| U32 | 74LS10 | **Crystal** | |
| U33 | 74LS74 | X1 | 5.0688 MHz |
| U34 | 74LS174 | | |
| U35 | 74LS138 | **Switches** | |
| U36 | 74LS04 | S1 | 8-Position DIP |
| U37 | 74LS125 | | |
| U38 | 74LS175 | **Connectors** | |
| U39-40 | 74LS38 | JA,JB | 26 Pin Transition |
| U41 | 74LS175 | JC,JD | 50 Pin Transition |
| U42 | 74LS125 | | |
| U43 | 74LS04 | **Jumpers** | |
| U44 | 74LS244 | J1-J2 | 16 Pin DIP Shunt |
| U45 | 74LS00 | J3-J14 | 12 Pin Post Assembly |
| U46-U47 | 74LS266R | J5-J16 | 16 Pin DIP Header |
| | | J17 | 3 Pin Post Assembly |
| | | (10) | Post Shunt |

**[sic]** "J5-J16 16 Pin DIP Header" overlaps "J3-J14 12 Pin Post Assembly". From the text, J5-J14
are post jumpers and J15-J16 are the 16-pin interrupt headers, so "J5-J16" is probably "J15-J16".
This is an interpretation.

Conversion notes:

- The logic diagram draws the TxINT/RxINT gates as U40/U54 (channels 0-3) and U39/U53 (channels
  4-7); the parts list gives U39-40 and U53-U54 as 74LS38 (open-collector NAND).
- The logic diagram labels the USARTs "2661 2651". The parts list gives "2651/61".
- The parts list gives R7-R8 as 1.2K. The logic diagram (page 31) labels them "R8 1K" and
  "R7 1K". Both are kept as printed.

## Component Layout (page 39)

A photograph of the component side, printed sideways: "COPYRIGHT 1981 GODBOUT ELECTRONICS
OAKLAND, CALIFORNIA, COMPONENT SIDE, MADE IN USA", "174C", "INTERFACER 3", "CompuPro". The eight
2651/61 USARTs (U21-U28) are in a row along the connector edge, with the 1488/1489 pairs, J1/J2
and the 26-pin and 50-pin connectors beside them. The regulators (U1 7912, U2 7812, U59 and U60
7805) are along the top edge. S1, J17, J18 and X1 are near the bus connector. Use the PDF for the
positions.

## Limited Warranty (page 40)

Viasyn Corporation warrants this computer product to be in good working order for a period of one
(1) year, (two (2) years for CSC boards and six (6) months for drives) from the date of purchase
by the original end user. Should this product fail to be in good working order at any time during
this warranty period, VIASYN will, at its option, repair or replace the product at no additional
charge except as set forth below. Repair parts and replacement products will be furnished on an
exchange basis and will be either reconditioned or new. All replaced parts and products become
the property of VIASYN. This limited warranty does not include service to repair damage to the
product resulting from accident, disaster, misuse, abuse, or unauthorized modification of the
product.

If you need assistance, or suspect an equipment failure, always contact your System Center or
dealer first. System Center technicians are factory trained to provide prompt diagnosis and
repair of equipment failures. If you are not satisfied by the actions taken by your System
Center/dealer, please call VIASYN at (415) 786-0909 to obtain a Return Material Authorization
(RMA) number, or, write to VIASYN at 3481 Arden Road, Hayward, California 94545, Att.: RMA. Be
sure to include a copy of the original bill of sale to establish purchase date. If the product is
delivered by mail or common carrier, you agree to insure the product or assume the risk of loss
or damage in transit, to prepay shipping charges to the warranty service location and to use the
original shipping container or equivalent. Contact your System Center/dealer or write to VIASYN
at the above address for further information.

ALL EXPRESSED AND IMPLIED WARRANTIES FOR THIS PRODUCT, INCLUDING THE WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE LIMITED IN DURATION TO THE ABOVE LISTED
PERIODS FROM THE DATE OF PURCHASE, AND NO WARRANTIES, WHETHER EXPRESSED OR IMPLIED, WILL APPLY
AFTER THIS PERIOD.

IF THIS PRODUCT IS NOT IN GOOD WORKING ORDER AS WARRANTED ABOVE, YOUR SOLE REMEDY SHALL BE REPAIR
OR REPLACEMENT AS PROVIDED ABOVE. IN NO EVENT WILL VIASYN BE LIABLE TO YOU FOR ANY DAMAGES,
INCLUDING ANY LOST PROFITS, LOST SAVINGS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF OR INABILITY TO USE SUCH PRODUCT, EVEN IF VIASYN OR A SYSTEM CENTER/DEALER HAS BEEN
ADVISED OF THE POSSIBILITY OF SUCH DAMAGES, OR FOR ANY CLAIM BY ANY OTHER PARTY.

IF THIS PRODUCT IS OUT OF WARRANTY, PLEASE CALL THE VIASYN RMA DEPARTMENT TO OBTAIN THE FLAT RATE
LABOR QUOTATION FOR FACTORY SERVICE.

Viasyn Corporation, 26538 Danti Court, Hayward, California 94545, Telephone (415) 786-0909, TWX
510-100-3288.

Note: This warranty supersedes all previous warranties, and all other warranties are now
obsolete.

Footer: "#17806/2", "Revised 4/85".

PDF page 45 is blank. PDF page 46 is the back cover: VIASYN, "The CompuPro People", 26538 Danti
Court, Hayward, CA 94545 (415) 786-0909; "8261-0015" at the lower left and "435200C/5" at the
lower right.

---

## What this means for the pascal100 model

A summary for the simulator. It interprets the manual; the sections above are the facts.

- **Ports:** 8 ports at a multiple of 8 (S1-2..6 = A7-A3; S1-1 ON disables). CompuPro software
  uses base 10H. Relative 0-3 are the selected USART's data, status (W: SYN/DLE), mode and
  command registers. Relative 4 and 5 read TxRDY0-7 and RxRDY0-7 and write the TxINTEN and
  RxINTEN masks. Relative 6 and 7 (A1 = A2 = 1) write the user select latch; reads there are not
  decoded.
- **User select:** a 5-bit latch, D0-D2 = relative user, D3-D4 = board. Every board at the same
  base latches every write. A board answers ports 0-5 only while D3-D4 match its S1-7/S1-8.
  Up to four boards share one 8-port block and give 32 users. Reset clears the latch to user 0,
  board 0.
- **Interrupts:** TxINTk = TxRDYk AND TxINTENk; RxINTk = RxRDYk AND RxINTENk, each jumpered (J15,
  J16) to any VI line, wire-ORed. Reset clears both masks. The status ports read the RDY lines
  whatever the masks. Channels 6 and 7 can add TxEMT/DSCHG to their Tx or Rx line (J13/J14,
  J3/J4).
- **The 2651:** the same USART as the System Support 1 (5.0688 MHz BRCLK, the same baud table and
  register formats). TxRDY is valid only while TxEN is set.
- **RS-232 inputs:** CTS, DSR and DCD are pulled up to +12V, so an unconnected input is ON. The
  2651 then sees CTS\* and DCD\* low (transmitter and receiver enabled), and SR-7/SR-6 read 1. So
  the streams' "missing line reads asserted" is right for this board.
- **Wait states:** J17 adds 0, 1 or 2 waits to USART accesses (relative 0-3) only.
- **CompuPro BIOS use:** `COMPUPRO.EQU` and `TMXINTIO.DVR` in the CP/M 8-16 1.1R source, and MP/M
  8-16 2.1E's `TTYS` file (Interfacer 3 ports 0-15, so two boards), are the software checks.
