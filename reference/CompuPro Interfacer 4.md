# CompuPro Interfacer 4 — Technical Manual

Converted from `Documentation/CompuPro/CompuPro Interfacer 4 Manual.pdf` (58 pages, a scan with
an Adobe Paper Capture layer). CompuPro, a Godbout company, board 187C, cover dated 5/83,
document #11877. Converted 2026-11-13.

Conversion method: every page was read from its page image. Small tables and drawings were read
at 300 dpi.

Conversion notes:

- **Printed page = PDF page + 1.** PDF page 1 is the cover; the printed page 2 (the inside of
  the cover) is not in the scan.
- Apparent errors in the printed manual are kept as printed and marked **[sic]**, with a short
  note.
- Photographs are described, not reproduced.

---

## Cover (PDF page 1)

INTERFACER 4™ TECHNICAL MANUAL. A photograph of the board. "IEEE 696 / S-100". "3 CHANNEL
SERIAL I/O BOARD with CENTRONICS and Universal Parallel Ports". "187C" in an arrow, with "mac"
below it. CompuPro®, A GODBOUT COMPANY. "5/83".

## Table of contents and disclaimer (pages 3-4)

| Section | Page |
|---|---|
| How to Get Your Interfacer 4 Board Up and Running in a CompuPro System in Five Minutes or Less Without Reading the Manual | 5 |
| HARDWARE SECTION | 7 |
| About Interfacer 4 | 7 |
| Technical Overview | 7 |
| Port Map | 8 |
| Port Addressing | 8 |
| User/Board Selection | 9 |
| User Select Register | 9 |
| Data Bus Select Switch S3 | 10 |
| Relative User 0 - 2 Swap Option | 11 |
| Wait State Selection | 11 |
| Cables | 11 |
| Using Interrupts | 13 |
| Interrupt Control Registers | 14 |
| Interrupt Status Registers | 15 |
| SERIAL INFORMATION SECTION | 16 |
| USART Initialization | 16 |
| USART Initialization Sequence | 16 |
| Data Registers | 17 |
| Status Registers | 17 |
| Mode Registers | 17 |
| Command Register | 18 |
| Serial Mode Jumpers | 19 |
| Programming Jumpers | 19 |
| RS-232C Control Lines | 19 |
| USART Handshaking Lines | 20 |
| Synchronous Mode Clock Driver/Receivers | 20 |
| Relative User 2 Synchronous Mode Jumpers | 21 |
| Relative User 3 Synchronous Mode Jumpers | 21 |
| Selecting Rate of On-Board Baud Rate Generator | 21 |
| UNIVERSAL PARALLEL CHANNEL | 22 |
| Technical Overview | 22 |
| I/O Address Assignment | 22 |
| Status-P Register Bit Assignment | 22 |
| Port Control Lines | 22 |
| Input Strobe Line | 22 |
| Input Examples | 23 |
| Output Enable Line | 23 |
| Attention Line | 23 |
| Output Examples | 24 |
| CENTRONICS STYLE PARALLEL CHANNEL | 25 |
| Technical Overview | 25 |
| Status-C Register Bit Assignment | 25 |
| Control-C Register Bit Assignment | 25 |
| Transmit Interrupt | 26 |
| Interface Signal Designations | 26 |
| Signal Differences between EPSON and CENTRONICS | 26 |
| Control-C Line Jumpering | 27 |
| Jumper Description | 27 |
| Light Emitting Diode | 28 |
| Sense Dip Switch | 28 |
| THEORY OF OPERATION | 29 |
| S-100 Bus Drivers | 29 |
| I/O Port Decode Logic | 29 |
| Strobe Generation Logic | 30 |
| Wait State Logic | 30 |
| Interrupt Control/Status Logic | 30 |
| USARTS | 31 |
| RS-232/Current Loop Level Conversion Logic | 31 |
| CENTRONICS Parallel Logic | 31 |
| Universal Parallel Logic | 32 |
| SOFTWARE SECTION | 33 |
| Sample Program Using Interfacer 4 as the CP/M Console | 33 |
| CENTRONICS Test Routine | 36 |
| Interfacer 4 Serial Test Routine | 39 |
| INS2651 Programmable Communications Interface | 41-47 |
| HARDWARE DESCRIPTION | 48 |
| Parts List | 48 |
| Component Layout | 49 |
| Logic Diagram | 50-57 |
| Jumper and Switch Index | 58 |
| CUSTOMER SERVICE / LIMITED WARRANTY INFORMATION | BACK PAGE |

Document #11877

DISCLAIMER: Godbout Electronics makes no representations or warranties with respect to the
contents hereof and specifically disclaims any implied warranties of merchantability or fitness
for any particular purpose. Further, Godbout Electronics reserves the right to revise this
publication and to make any changes from time to time in the content hereof without obligation
of Godbout Electronics to notify any person of such revision or changes.

This document was proofread with the aid of SpellGuard™ from SORCIM, Santa Clara, CA.

## How to Get Your Interfacer 4 Board Up and Running in a CompuPro System in Five Minutes or Less Without Reading the Manual (pages 5-6)

This section allows the user to configure an INTERFACER 4 in a standard CompuPro system running
CP/M-80 or CP/M-86 so that the INTERFACER 4 DRIVES the console, list and UL1 devices. If, after
reading and following the directions in this section, your board appears not to function, or if
you are planning to use this board in other than a standard CompuPro system, DON'T CALL!!! READ
THE MANUAL FIRST!!!

### Switches

DIP SWITCH S1: This switch is not used by either the CP/M-80 or the CP/M-86 BIOS, so we
recommend that you turn all positions "OFF".

DIP SWITCH S2: This switch controls the port addressing and board selection number for the
board. It should be set as described in the table below. These settings will put the
INTERFACER 4 at ports 10-17H as users 4-7.

| Position | Labeled | How to set it |
|---|---|---|
| 1 | BS0 | ON |
| 2 | BS1 | ON |
| 3 | H/L | OFF |
| 4 | DIS | OFF |
| 5 | A7 | ON |
| 6 | A6 | ON |
| 7 | A5 | ON |
| 8 | A4 | OFF |
| 9 | A3 | ON |
| 10 | - | ON (NOT USED) |

DIP SWITCH S3: This switch should be set with positions 1-4 "ON", and positions 5-8 "OFF".

### Jumper sockets

The jumper sockets should have either an 8 position shunt or an 8 position DIP header as
indicated below.

| Jumper socket | Setting |
|---|---|
| JS1 | SHUNT INSTALLED |
| JS2 | SHUNT INSTALLED |
| JS3 | SHUNT INSTALLED |
| JS4 | HEADER INSTALLED WITH NO WIRES |
| JS5 | HEADER INSTALLED WITH NO WIRES |
| JS6 | HEADER INSTALLED WITH NO WIRES |

### Jumpers (page 6)

The pin shunt jumpers should be installed or removed as indicated.

| Jumper | Condition |
|---|---|
| J1 | NO PINS NEED TO BE INSTALLED |
| J2 | BOTTOM INSTALLED WITH EPSON / NO SHUNT OTHERWISE |
| J3 | TOP INSTALLED WITH EPSON / NO SHUNT OTHERWISE |
| J4 | BOTTOM INTSALLED [sic] WITH BOTH EPSON AND CENTRONICS |
| J5 | REMOVED |
| J6 | INSTALLED FOR 1 WAIT STATE / REMOVED OTHERWISE |
| J7 | INSTALLED FOR 2 WAIT STATES / REMOVED OTHERWISE |
| J8 | INSTALLED FOR 3 WAIT STATES / REMOVED OTHERWISE |
| J9 | NO JUMPER |
| J10-J25 | REMOVED |
| J26 | JUMPER A-B and C-D for the CENTRONICS CHANNEL as USER 4. JUMPER A-C and B-D for the CENTRONICS CHANNEL as USER 6. |

DIAGRAM - JUMPER AND JUMPER SOCKET LAYOUT: a drawing of the board ("187A") with the IC
positions numbered and the jumpers marked: PARALLEL CONN 1, CENTRONICS CONN 2 and TRIPLE SERIAL
CONN 3 (A, B, C) along the top edge; JS1-JS4 above the USARTs; J10-J12 below them; J13-J24 at the
right; S1 at the left, S2 at the lower left; J6-J8 beside U20.

NOTE: Under MP/M 8-16 or an interrupt driven mode, J26 MUST be jumpered A-B and C-D.

## Hardware Section (page 7)

### About Interfacer 4

Congratulations on your decision to purchase the INTERFACER 4 multi-purpose I/O board.
INTERFACER 4 has been designed to be the most flexible and highest performance I/O interface
available that fully complies with the IEEE 696/S-100 bus standard. Due to its provision for
ready expansion and modification as the state of the computing art improves, the S-100 bus is
the professional level choice for commercial, industrial, and scientific applications. We
believe that this board, along with the rest of the S-100 portion of the CompuPro family, is one
of the best boards available for that bus.

The INTERFACER 4 boasts several innovative features not found on currently available I/O
boards. The primary innovation stems from its full software compatibility with the INTERFACER 3
as well as the ability to intermix INTERFACER 3 and 4 boards at the same port addresses.
Additional features include 3 fully programmable asynchronous serial channels, 2 of which are
capable of high speed synchronous transmission and one capable of current loop operation, five
RS-232 handshaking lines per channel plus bi-directional clock drivers on both the synchronous
channels, a pin compatible CENTRONICS parallel interface port with the full complement of
handshaking lines, a universal parallel port with 16 data and 3 handshaking lines,
expandability to 32 users with eight boards using only 8 port addresses, a flexible interrupt
structure with full maskability and pending status on both transmit and receive interrupts, and
conservative design for operation with most CPUs operating to beyond 10 MHz. Other features
standard to all CompuPro boards include thorough bypassing of all supply lines to suppress
transients, on-board regulators, and low power Schottky TTL and MOS technology integrated
circuits for reliable, cool operation. All this and sockets for all IC's go onto a double sided,
solder masked printed circuit board with a complete component legend.

### Technical Overview (pages 7-8)

The INTERFACER 4 was designed for efficient operation in interrupt driven/multi-user
microcomputer systems as well as polled mode single user systems. Eight distinct interrupts are
generated on-board by the three USARTs and two parallel ports, and these are brought out for
jumpering by the user to the eight vectored interrupt lines on the S-100 bus. Since these
interrupt lines are open collector, they may be configured to interrupt on any or all of the
vectored interrupt lines. In addition, a transmit and receive interrupt mask port is provided
for inhibiting unwanted interrupts.

The INTERFACER 4 provides multi-user operation with a minimum number of I/O ports by
incorporating a user select register to activate the required I/O channel. This five bit
register is used to select a particular channel, which allows up to 32 users (up to eight
boards) on the same 8 port addresses. When a particular user is selected, the four USART
registers associated with that specific serial channel or the parallel registers are made
available for examination and alteration by the host processor or other temporary bus master.
In addition, whenever a particular channel is selected, the interrupt registers on that
particular board as well as the registers on another board in the same group of eight users are
available for examination and alteration.

The typical sequence of operation would require all channels on the INTERFACER 4 to be mode
initialized and the interrupt mask registers set for operation. All parameters of the USART or
parallel ports may be altered by selecting that particular channel and writing a new set of
mode and command words to the proper registers. If running in a non-interrupt environment, the
interrupt status registers may be polled and checked in roughly the same manner as a standard
single channel serial board.

All three of the serial channels on the INTERFACER 4 are designed for direct connection to DATA
TERMINAL EQUIPMENT (DTE) or DATA COMMUNICATION EQUIPMENT (DCE) in asynchronous mode without
alteration of the cables. This allows direct connection to all types of RS-232 equipment
including modems. In addition, two channels are capable of high speed synchronous operation
using internal or external clocks and one channel may be connected to current loop devices.

The CENTRONICS parallel channel was designed for direct connection to printers using standard
ribbon cable connectors. In addition, all handshaking lines have been implemented for maximum
flexibility and ease of interfacing.

### Port Map (page 8)

The INTERFACER 4 interface uses a block of eight port addresses for communication between it and
the host processor. The address of the first port is switch selectable to any address which is
a multiple of eight. The ports will be referred to as RELATIVE PORTS 0 - 7.

| Relative port | Function |
|---|---|
| 0 | USART / CENTRONICS / DIPSWITCH DATA-C Register (R/W) |
| 1 | USART / CENTRONICS STATUS-C Register (R); SYN1/SYN2/DLE Register / CENTRONICS CONTROL-C Reg. (W) |
| 2 | USART Mode Register / Parallel DATA-P Register (R/W) |
| 3 | USART Command Register / Parallel STATUS-P Register (R/W) |
| 4 | Transmit Interrupt Status Register (R); Transmit Interrupt Mask Register (W) |
| 5 | Receive Interrupt Status Register (R); Receive Interrupt Mask Register (W) |
| 6 | Not used |
| 7 | User Select Register (write only) |

### Port Addressing (pages 8-9)

DIP switch S2, positions 4 thru 9 are used to select the base address of the eight port block in
a binary fashion as shown in the following table:

| Switch position | Address bit |
|---|---|
| 4 | PORT DISABLE WHEN "ON" |
| 5 | A7 |
| 6 | A6 |
| 7 | A5 |
| 8 | A4 |
| 9 | A3 |

"ON" = "0", "OFF" = "1".

EXAMPLE: To address this board at addresses 10H thru 17H for the CompuPro CP/M-80 or CP/M-86
operating system or the Phase 1 OASIS operating system, position 4 and 8 would be "OFF" and
positions 5 thru 7 and positions 9 would be "ON".

### User/Board Selection (pages 9-10)

To select a particular channel and to select which board that channel will be on (when running
more than 4 users), requires the use of the User Select Port and three board select switches.
The five bit User Select Register determines which of 32 possible users will be selected at a
particular time. The two board select switches (S2-1 and S2-2) determine whether a board will
respond to users 0 thru 7, 8 thru 15, 16 thru 23, and 24 thru 31 and the HIGH/LOW select switch
(S2-3) determines whether the board is the high or low 4 users in a particular block of eight. A
particular user (0-31) is selected by outputting the five bit number that represents that user.
The diagram shown below describes the relation between the board select switches and the User
Select Register.

USER SELECT REGISTER

| Data bit | Name | Function |
|---|---|---|
| D0 | US0 | USER SELECT 0 (LSB) |
| D1 | US1 | USER SELECT 1 |
| D2 | H/LS | HIGH/LOW SELECT |
| D3 | BS0 | BOARD SELECT 0 (LSB) |
| D4 | BS1 | BOARD SELECT 1 (MSB) |
| D5 | | NOT USED |
| D6 | | NOT USED |
| D7 | | NOT USED |

Since each INTERFACER 4 will support 4 users, we will refer to these 4 as RELATIVE USERS 0-3.
These 4 ports are physically configured with RELATIVE USER 0 as the CENTRONICS and Universal
Parallel ports, RELATIVE USER 1 as the far right serial channel with current loop capabilities
(CONN 3 C), RELATIVE USER 2 is the middle channel (CONN 3 C) [sic], and RELATIVE USER 3 as the
far left channel (CONN 3 A).

**[sic]** The middle channel is CONN 3 B (see the layout on page 6: CONN 3 A, B, C from left to
right).

To determine the EXACT USER number, the RELATIVE USER number must be added to the USER OFFSET
number. The RELATIVE USER number corresponds to the 2 bits above called USER SELECT 0-1, and the
USER OFFSET number corresponds to the 3 bits above called BOARD SELECT 0 and 1, and HIGH/LOW
SELECT. These 5 bits determine the exact user number.

| US1 | US0 | Relative user number | Standard configuration |
|---|---|---|---|
| 0 | 0 | USER 0 | CENTRONICS / PARALLEL |
| 0 | 1 | USER 1 | RIGHT SERIAL CHANNEL |
| 1 | 0 | USER 2 | MIDDLE SERIAL CHANNEL |
| 1 | 1 | USER 3 | LEFT SERIAL CHANNEL |

| S2-3 | S2-2 | S2-1 | H/LS | BS1 | BS0 | User offset |
|---|---|---|---|---|---|---|
| ON | ON | ON | 0 | 0 | 0 | 0 |
| OFF | ON | ON | 1 | 0 | 0 | 4 |
| ON | ON | OFF | 0 | 0 | 1 | 8 |
| OFF | ON | OFF | 1 | 0 | 1 | 12 |
| ON | OFF | ON | 0 | 1 | 0 | 16 |
| OFF | OFF | ON | 1 | 1 | 0 | 20 |
| ON | OFF | OFF | 0 | 1 | 1 | 24 |
| OFF | OFF | OFF | 1 | 1 | 1 | 28 |

### Data Bus Select Switch S3 (page 10)

Switch S3 is used to steer either the high (D4-D7) or the low (D0-D3) nibble of status/control
information to and from the interrupt logic on the INTERFACER 4. This switch was designed in to
provide software compatibility with the INTERFACER 3 board.

When the INTERFACER 4 is addressed as EXACT USERS 0-3 + N, (where N is 0, 8, 16, or 24) we would
like the low nibble (D0-D3) of status and control information to be used. This would require
switch S3 to have positions 1-4 "OFF" and positions 5-8 "ON".

When the INTERFACER 4 is addressed as EXACT USERS 4-7 + N, (where N is 0, 8, 16, or 24) we would
like the high nibble (D4-D7) of status and control information to be used. This would require
switch S3 to have positions 1-4 "ON" and positions 5-8 "OFF".

NOTE!: SETTING SWITCH S3 DIFFERENTLY THAN DESCRIBED ABOVE WILL CAUSE IMPROPER BOARD OPERATION AND
POSSIBLE BOARD DAMAGE!

EXAMPLE: To address the INTERFACER 4 to respond to EXACT USERS 4 thru 7 (the CompuPro standard),
switches S2-1 and S2-2 would be "ON", and S2-3 would be "OFF". To select a particular user in the
group from 4 to 7, BS1 (D4) and BS0 (D3) of the User Select Register must be "0", and H/LS (D2)
must be "1" for the board to respond. Switch S3 must have positions 1-4 "ON" and 5-8 "OFF". To
select EXACT USER 5, a 05H must be sent to the USER SELECT REGISTER.

EXAMPLE: To address the INTERFACER 4 to respond to users 16 thru 19, switch S2-1 and S2-3 would
be "ON", and switch S2-2 would be "OFF". Switch S3 must have positions 1-4 "OFF" and 5-8 "ON". To
select a particular user in the group from 16 to 19, BS1 must be a "1" , BSO must be "0", and
H/LS must be a "0" for the board to respond. To select EXACT USER 18, a 12H must be sent to the
USER SELECT REGISTER.

### Relative User 0 - 2 Swap Option (page 11)

The INTERFACER 4 may be configured so that RELATIVE USERS 0 and 2 may be swapped by re-jumpering
J26. This will configure the CENTRONICS and UNIVERSAL Parallel channels as RELATIVE USER 2
instead of 0, and the middle serial channel as RELATIVE USER 0 instead of 2. This option allows
EXACT USER 6 to be either a serial channel or the CENTRONICS channel for compatibility with the
standard CompuPro CP/M BIOS. (This allows the LPT LIST device to be either serial or parallel
without changing the BIOS.)

The standard configuration has "A" connected to "B", and "C" connected to "D" on J26. This
provides the CENTRONICS as RELATIVE USER "0". To swap this, jumper "A" to "C", and "B" to "D" on
J26, and the CENTRONICS channel will be RELATIVE USER 2 and the middle serial channel will be
RELATIVE USER 0.

**[sic]** Page 6 says J26 A-B/C-D gives "the CENTRONICS CHANNEL as USER 4" and A-C/B-D "as USER
6". With the board at users 4-7, relative user 0 is exact user 4 and relative user 2 is exact
user 6, so the two pages agree; page 6 gives exact user numbers.

### Wait State Selection (page 11)

The INTERFACER 4 was designed to run in very fast microcomputer systems by allowing up to three
wait states to be added when accessing the USART/PARALLEL registers. Since the user select and
interrupt control registers are capable of higher speed operation than the USART registers, no
wait states are inserted even when they are enabled on the board.

The 3 sets of vertical pins (J6, J7, and J8) control the enabling of one, two, or three wait
states. With the black pin shunt on J6, one wait state will be inserted. With the pin shunt on
J7, two wait states will be inserted. With the pin shunt on J8, three wait states will be
inserted. If the pin shunt is left removed, no wait states will be inserted.

NOTE: If multiple INTERFACER 4 boards are inserted, they should be set to the same number of
wait states.

### Cables (pages 11-12)

The INTERFACER 4 is designed to use 3 different cable assemblies. The serial channels use a
custom 50 conductor cable that splits into three DB-25S connectors. The Universal Parallel
channel uses a standard 26 conductor cable identical to those used on the INTERFACER 1 and
INTERFACER 2, and the CENTRONICS Parallel channel uses a custom 40 pin cable that mates to a 36
pin "D" Shell connector for interfacing to the printer.

The serial channels (CONN 3, 50 pin connector on the far right) use a custom 3 user cable (see
photo A page 12). This cable consists of a female 50 pin insulation displacement connector that
splits into thirds and connects to three female DB-25 connectors. The actual cable has positions
1-16 (pin 1 on the far left side of the connector) on the first DB-25, positions 17-32 on the
second DB-25, and positions 33-50 on the third DB-25. NOTE: The pin numbers on the circuit
diagram show the pin numbers on the DB-25 connector and not the 50 pin connector.

The Universal Parallel channel (CONN 1, 26 pin connector on the far left) uses the CompuPro
standard RS-232 I/O cables (see photo B page 12). This cable consists of a female 26 pin
insulation displacement connector that mates to a
female DB-25 (the 26th conductor is not used). NOTE: The pin numbers on the circuit diagram show
the pin numbers on the DB-25 connector and not the 26 pin connector.

The CENTRONICS Parallel channel (CONN 2, 40 pin connector in the middle of the board) uses
another custom cable (see photo C page 12). This cable consists of a 40 pin female transition
connector that mates with a 36 pin female "D" shell connector identical to those on the back of
CENTRONICS interface printers. NOTE: The pin numbers on the circuit diagram show the pin numbers
on the D-36 connector and not the 40 pin connector, and that pin 1 of the 40 pin connector does
not correspond to pin 1 of the D-36 connector. If the user wishes to make this cable, the 36
conductors from the D-36 connector should be centered in the 40 pin connector, with 2 unused
pins on each edge.

Page 12 photographs: **A**, the 50 pin connector splitting into three DB-25 connectors; **B**,
the 26 pin connector with one DB-25; **C**, the 40 pin connector with a 36 pin Centronics
connector.

### Using Interrupts (pages 13-14)

The INTERFACER 4 has a simple but elegant interrupt structure that allows considerable
flexibility. Each USART generates both a transmit and receive interrupt. The CENTRONICS Channel
generates an interrupt upon receiving an ACKNOWLEDGE from the printer, and the Universal
Parallel channel generates an interrupt after being STROBED by an external device for a total of
8 distinct interrupts for the board. A transmit interrupt indicates that the USART or the
CENTRONICS transmit register is empty and it is ready to accept a character. A receive interrupt
indicates that data is available from the receiver data register on either the USART or the
Universal Parallel channel. Each of these interrupts may be masked "OFF" or "ON" by altering the
INTERRUPT CONTROL REGISTERS as described below. Each of these interrupts are open collector, and
may be individually tied to any of the 8 vectored interrupt lines (VI0-VI7). The status of each
interrupt line may be sampled by reading the INTERRUPT STATUS REGISTERS as described below.

Since each of the 8 interrupts generated on the INTERFACER 4 may be tied to any of the 8
vectored lines, almost any type of priority scheme may implemented. All transmit interrupts are
brought out twice on one side of jumper socket JS5, and all receive interrupts are brought out
twice on one side of jumper socket JS6. On the opposite side of each socket, each of the 8
vectored interrupt lines are brought out. By using the provided headers, any USART interrupt may
be connected to any VI line. The pin-out of JS5 and JS6 are shown below.

| Interrupt | JS5 pin | JS5 pin | VI line | JS6 pin | JS6 pin | Interrupt |
|---|---|---|---|---|---|---|
| TxINT 0 | 9 | 8 | VI0 | 8 | 9 | RxINT 0 |
| TxINT 1 | 10 | 7 | VI1 | 7 | 10 | RxINT 1 |
| TxINT 2 | 11 | 6 | VI2 | 6 | 11 | RxINT 2 |
| TxINT 3 | 12 | 5 | VI3 | 5 | 12 | RxINT 3 |
| TxINT 0 | 13 | 4 | VI4 | 4 | 13 | RxINT 0 |
| TxINT 1 | 14 | 3 | VI5 | 3 | 14 | RxINT 1 |
| TxINT 2 | 15 | 2 | VI6 | 2 | 15 | RxINT 2 |
| TxINT 3 | 16 | 1 | VI7 | 1 | 16 | RxINT 3 |

EXAMPLE: If we wish to generate an interrupt on vectored interrupt line VI3 when data becomes
available from RELATIVE USER 3, a wire should be soldered between pins 5 and 12 of JS6.

EXAMPLE: If we wish to generate an interrupt on vectored interrupt line VI6 when data becomes
available from RELATIVE USERS 0, 1, 2, and 3, a wire should be soldered to connect pins 1,
13,14,15, and 16 of JS6. **[sic]** By the table, VI6 is JS6 pin 2; pin 1 is VI7.

EXAMPLE: If we wish to generate an interrupt on vectored interrupt line VI0 when RELATIVE USER 2
is ready to accept a character, a wire should be soldered to connect pins 8 and 11 of JS5.

All serial channels are capable of generating a third interrupt called TxEMT/DSCHG\*. This
interrupt occurs when the transmitter has completed serialization of the last character loaded
or a change has occurred in the state of the DSR or DCD RS-232 status lines. Additional
information on this line may be found in the 2651 data sheet in this manual.

The TxEMT/DSCHG\* output from the 2651 may be jumpered to generate either a transmit or receive
interrupt. Due to the wire-OR capability of the interrupt outputs from the 2651, when jumpered,
the transmit interrupt will become TxRDY OR TxEMT/DSCHG\* or the receive interrupt will become
RxRDY OR TxRDY/DSCHG\* [sic]. Therefore, when jumpered, the user must check the status register to
determine what condition caused the interrupt.

**[sic]** "TxRDY/DSCHG\*" in the receive case: TxEMT/DSCHG\* is meant.

The following table will demonstrate where to install the shorting plug to generate the
appropriate interrupt.

| Channel number | To cause a TxEMT/DSCHG interrupt on the TxRDY line | On the RxRDY line |
|---|---|---|
| 1 | INSTALL J10T | INSTALL J10R |
| 2 | INSTALL J11T | INSTALL J11R |
| 3 | INSTALL J12T | INSTALL J12R |

### Interrupt Control Registers (pages 14-15)

Two registers are provided for individually masking the transmit and receive interrupts from the
bus. On power-up or reset, all interrupts are disabled on the INTERFACER 4. Alteration of the
interrupt registers may be accomplished in groups of eight users for compatibility with the
INTERFACER 4 [sic]. To gain access to these registers, a user channel must be enabled in the
particular group of 8 users. (You cannot alter any interrupt register on a pair of boards set for
users 0 thru 7 unless you have selected one of those 8 users)

**[sic]** "compatibility with the INTERFACER 4": the Interfacer 3 is meant.

If an INTERFACER 4 is installed in a system where it is selected as EXACT USERS 0-3 + "N", where
"N" is 0, 8, 16, or 24 (i.e. users 0-3, 8-11, etc.), a Transmit or Receive interrupt may be
enabled by outputting a "1" to the proper bit of the appropriate register. The registers are
configured so that Data Bit 0 will mask RELATIVE USER 0, D1 will mask RELATIVE USER 1, D2 will
mask RELATIVE USER 2, and D3 will mask RELATIVE USER 3.

If an INTERFACER 4 is installed in a system where it is selected as EXACT USERS 4-7 + "N", where
"N" is 0, 8, 16, or 24 (i.e. users 4-7, 12-15, etc.), a Transmit or Receive interrupt may be
enabled by outputting a "1" to the proper bit of the appropriate register. The registers are
configured so that Data Bit 4 will mask RELATIVE USER 4, D5 will mask RELATIVE USER 5, D6 will
mask RELATIVE USER 6, and D7 will mask RELATIVE USER 7. This is true for both the Transmit
Interrupt Control Register (relative port 4) and the Receive Interrupt Control Register
(relative port 5).

EXAMPLE: To enable all Transmit Interrupts on a particular INTERFACER 4, you should send to
relative port 4 either a 0FH if the board is selected as a 0-3 group or send a 0F0H if the board
is selected as a 4-7 group.

EXAMPLE: To enable the Transmit Interrupt on relative users 1, 4 and 6 in a pair of INTERFACER 4
boards configured as a group of eight users, you should send a 52H to relative port 4.

EXAMPLE: To disable all Receive Interrupts on a particular INTERFACER 4 selected as a 4-7 group,
you should send a 0FH to relative port 5.

EXAMPLE: To enable the Receive Interrupt on relative users 2, 3 and 7 in a pair of INTERFACER 4
boards, you should send a 8CH to relative port 5.

### Interrupt Status Registers (page 15)

Two registers are provided for checking the status of pending transmit and receive interrupts.
To gain access to these registers, a user channel must be enabled on the particular board or
pair of boards in a group of eight users to be altered. The INTERFACER 4 board has the
intelligence to allow you to read the interrupt status from a pair of boards simultaneously.
(You cannot read any interrupt register on a pair of boards set for users 0 thru 7 unless you
have selected one of those 8 users).

If a Transmit or Receive interrupt is pending, a "1" will be present in the proper bit of the
status register. The registers are configured so that Data Bit 0 contains the status of EXACT
USER 0+N, D1 contains the status of EXACT USER 1+N, and so on with D7 containing the status of
EXACT USER 7+N, where N is 0, 8, 16, or 24. This is true for both the Transmit Interrupt Status
Register (relative port 4) and the Receive Interrupt Status Register (relative port 5).
Remember, these status registers are read only! Writing into these registers will alter the
Interrupt Control Mask. In addition, the status of a channel's interrupts are available even if
those interrupts are masked "OFF". The Interrupt Control Register does not affect the reading of
the status from a register.

EXAMPLE: If all Transmit Interrupts on a particular pair of INTERFACER 4 boards are asserted,
you will read a 0FFH at relative port 4.

EXAMPLE: If Transmit Interrupts are pending on EXACT USERS 1, 4 and 6 (+N), you will read a 52H
from relative port 4.

EXAMPLE: If there are no Receive Interrupts pending on a single INTERFACER 4 in a system, (no
data available), you will read either a 0F0H from relative port 5 if the board is set for EXACT
USERS 0-3 (+N), or you will read a 0FH if the board is set for EXACT USERS 4-7 (+N). The reason
for the nibble of value "F" is the processor will read binary "1"s from non-driven lines.

EXAMPLE: If Receive Interrupts are pending on EXACT USERS 2 and 3 (+N) with a single INTERFACER 4
in the system, you will read a 0FCH from relative port 5.

## Serial Information Section (pages 16-21)

### USART Initialization (page 16)

The serial channels on the INTERFACER 4 are implemented with a 2651 type USART from either
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

A table of the various registers and where they appear in the I/O port map is shown in a
previous section and in the following tables.

"READ" or "INPUT" Ports

| Relative port address | UART register function |
|---|---|
| 00 hex | Data Port, read received data. |
| 01 hex | Status Port, read UART status info. |
| 02 hex | Mode Registers, read current UART mode. |
| 03 hex | Command Register, read current command. |

"WRITE" or "OUTPUT" Ports

| Relative port address | UART register function |
|---|---|
| 00 hex | Data port, write transmit data. |
| 01 hex | SYN1/SYN2/DLE register, write sync bytes. |
| 02 hex | Mode registers, write mode bytes. |
| 03 hex | Command register, write command byte. |

### USART Initialization Sequence (pages 16-17)

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

### Data Registers (page 17)

The USART data registers are straight-forward in their operation. You write a byte to the data
register when you want to transmit that byte to an external serial device and you read the byte
in the data register to receive a byte from an external serial device. The USART will
automatically add the proper start and stop bits when transmitting and will remove them when
receiving.

### Status Register (page 17)

The status register is used to determine the current state of the USART. Each bit of the status
register has a different meaning depending on whether it is high or low. (High means a logic one
or high level and low means a logic zero or low level.) The following table describes the
meaning of the status bits:

STATUS REGISTER FORMAT

| Bit | Name | Meaning |
|---|---|---|
| SR-7 | DATA SET READY | 0 = DSR\* input is high; 1 = DSR\* input is low |
| SR-6 | DATA CARRIER DETECT | 0 = DCD\* input is high; 1 = DCD\* input is low |
| SR-5 | FE/SYN DETECT | ASYN: 0 = normal, 1 = framing error. SYNC: 0 = normal, 1 = SYN character detected |
| SR-4 | OVERRUN | 0 = normal, 1 = overrun error |
| SR-3 | PE/DLE DETECT | ASYNC: 0 = normal, 1 = parity error. SYNC: 0 = normal, 1 = parity error or DLE character received |
| SR-2 | TxEMT/DSCHG | 0 = normal; 1 = change in DSR or DCD, or transmit shift register is empty |
| SR-1 | RxRDY | 0 = receive holding register empty; 1 = receive holding register has data |
| SR-0 | TxRDY | 0 = transmit holding register busy; 1 = transmit holding register empty |

NOTE 1. BAUD RATE FACTOR IN ASYNCHRONOUS MODE APPLIES ONLY IF EXTERNAL CLOCK IS SELECTED. FACTOR
IS 16x IF INTERNAL CLOCK IS SELECTED.

### Mode Registers (pages 17-18)

When bringing up the USART, its two mode registers must be set with various bit patterns that
will determine the operating modes. Although there are two registers, they occupy only one I/O
port address. This is accomplished with internal sequencing logic that allows you to write the
first register (Mode Register 1) and then the second register (Mode Register 2). It is important
to write to Mode Register 1 first.

The meanings of the various bits in the mode registers are described in the following tables:

MODE REGISTER 1 FORMAT

| Bits | Function |
|---|---|
| MR1-7, MR1-6 | SYNC: MR1-7 number of SYN characters (0 = double SYN, 1 = single SYN); MR1-6 transparency control (0 = normal, 1 = transparent). ASYNC: stop bit length (00 = invalid, 01 = 1 stop bit, 10 = 1½ stop bits, 11 = 2 stop bits) |
| MR1-5 | Parity type: 0 = odd, 1 = even |
| MR1-4 | Parity control: 0 = disabled, 1 = enabled |
| MR1-3, MR1-2 | Character length: 00 = 5 bits, 01 = 6 bits, 10 = 7 bits, 11 = 8 bits |
| MR1-1, MR1-0 | Mode and baud rate factor (note 1): 00 = synchronous 1x rate, 01 = asynchronous 1x rate, 10 = asynchronous 16x rate, 11 = asynchronous 64x rate |

MODE REGISTER 2 FORMAT

| Bits | Function |
|---|---|
| MR2-7, MR1-6 [sic] | Not used |
| MR2-5 | Transmitter clock: 0 = external, 1 = internal |
| MR2-4 | Receiver clock: 0 = external, 1 = internal |
| MR2-3..0 | Baud rate selection: 0000 = 50, 0001 = 75, 0010 = 110, 0011 = 134.5, 0100 = 150, 0101 = 300, 0110 = 600, 0111 = 1200, 1000 = 1800, 1001 = 2000, 1010 = 2400, 1011 = 3600, 1100 = 4800, 1101 = 7200, 1110 = 9600, 1111 = 19200 |

**[sic]** The Mode Register 2 table heads bit 6 "MR1-6"; MR2-6 is meant.

That completes the description of the Mode Registers. Remember that you must always write both
mode registers, with Mode Register 1 first.

### Command Register (page 18)

The Command Register is used to set the operating mode (sync or async), enable or disable the
receiver and/or transmitter, force a "break" condition, reset the error flags and control the
state of the RTS and DTR outputs.

COMMAND REGISTER FORMAT

| Bits | Function |
|---|---|
| CR-7, CR-6 | Operating mode: 00 = normal operation; 01 = async: automatic echo mode, sync: SYN and/or DLE stripping mode; 10 = local loop back; 11 = remote loop back |
| CR-5 | Request to send: 0 = forces RTS\* output high, 1 = forces RTS\* output low |
| CR-4 | Reset error: 0 = normal, 1 = reset error flag in status register (FE, OE, PE/DLE detect) |
| CR-3 | ASYNC: force break (0 = normal, 1 = force break). SYNC: send DLE (0 = normal, 1 = send DLE) |
| CR-2 | Receive control (RxEN): 0 = disable, 1 = enable |
| CR-1 | Data terminal ready: 0 = forces DTR\* output high, 1 = forces DTR\* output low |
| CR-0 | Transmit control: 0 = disable, 1 = enable |

### Serial Mode Jumpers (page 19)

The INTERFACER 4 board with its serial programming jumpers allows the user to adapt all three
channels to all standard RS-232 pin configurations and Relative User 1 to standard current loop
configurations. In RS-232 mode, these jumpers may be set so that this board operates in a
"master" mode where it behaves as the Data Terminal Equipment (DTE), or it may be set so that the
board operates in a "slave" mode where it behaves as the Data Communication Equipment (DCE).
With almost all CRT terminals and serial interface printers, the INTERFACER 4 serial mode
jumpers (JS1-JS3) must be set in the "slave" or DCE mode. When connected to a Modem, the serial
mode jumpers (JS1-JS3) of the INTERFACER 4 should be set in the "master" mode as shown on the
following table. In current loop mode on Relative User 1, JS3 should be removed and JS4 and J25
installed. The proper configuration of JS4 depends on whether the on-board or an off-board 20mA
current source is used. The wiring of the mating DB-25 connector should have pins 14 and 15 as
the + and - inputs, and 16 and 17 as the + and - outputs. For special applications, pins 18 and
19 of the DB-25 are TTL IN and OUT if pin 7 and 10, and 8 and 9 of JS4 are shorted.

### Programming Jumpers (page 19)

Four drawings of a 16-pin jumper header, pin 1 at the top left:

- **SLAVE MODE, JS1-JS3:** for connections to CRT terminals, printers, etc. Eight straight
  shunts, each pin joined to the pin opposite.
- **MASTER MODE, JS1-JS3:** for connection to MODEMS. The top three pairs of rows crossed, the
  last rows straight.
- **CURRENT LOOP - on board current source, JS4:** Example TTY. Two crossed loops.
- **CURRENT LOOP - external current source, JS4.** Diagonal wires.

The exact wire paths of the drawings are not stated in the text.

### RS-232C Control Lines (pages 19-20)

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

Five RS-232 handshaking signals are provided for interfacing to equipment needing these lines as
shown below. Output lines may be set either "MARKING" or "SPACING" and their state may be altered
by software commands as described in the USART INITIALIZATION Section under Command Register.

### USART Handshaking Lines (page 20)

| | Name | RS-232 line | DB25 pin connection |
|---|---|---|---|
| Output | DTR | CD | 20 OR 6 \* |
| Output | RTS | CA | 4 OR 5 \* |
| Input | DSR | CC | 6 OR 20 \* |
| Input | CTS | CB | 5 OR 4 \* |
| Input | DCD | CF | 8 |

\* NOTE: Pin numbers with no asterisk indicate the DB25 pin number when the Serial Mode Jumpers
are set for "master" mode. Pin numbers with an asterisk indicate the DB25 pin number when the
Serial Mode Jumpers are set for "slave" mode.

### Synchronous Mode Clock Driver/Receivers (page 20)

RELATIVE CHANNELS 2 and 3 can either transmit or receive the synchronous timing element signals.
The typical configuration requires that the DATA COMMUNICATION EQUIPMENT (DCE) be the source of
the of [sic] the synchronous transmit and receive clocks. The INTERFACER 4 is capable of
independently transmitting or receiving the sync clocks in either DCE or DTE modes.

For using either channel in a synchronous mode, there are two major options. The first option is
whether or not you will be using the USART internal baud rate generator or the on-board high
speed baud rate generators. The second option is whether you will be transmitting the sync
clocks to the external device or receiving them from the external device. The following table
will describe how each pin shunt should be set.

### Relative User 2 Synchronous Mode Jumpers (page 21)

| | Transmitting sync clocks | Receiving sync clocks |
|---|---|---|
| Internal baud rate use - low speed operation | INSTALL J13, J16 | INSTALL J15, J21 |
| External baud rate use - high speed operation | INSTALL J13, J14, J16; INSTALL J23 FOR DESIRED RATE | INSTALL J15, J21 |

### Relative User 3 Synchronous Mode Jumpers (page 21)

| | Transmitting sync clocks | Receiving sync clocks |
|---|---|---|
| Internal baud rate use - low speed operation | INSTALL J17, J20 | INSTALL J19, J22 |
| External baud rate use - high speed operation | INSTALL J17, J18, J20; INSTALL J24 FOR DESIRED RATE | INSTALL J19, J22 |

### Selecting the Rate of the On-Board Baud Rate Generator (page 21)

Either or both of the synchronous channels may use the on-board high speed baud rate generator
for communication at rates greater than that available from the USART. The rates available
on-board include 31.25K, 62.50K, 125K, 250K, and 500K baud, however, it is unlikely that 500K
baud will be usable in most applications due to the slew rate limitations of the RS-232 drivers
and receivers. The table below describes the jumper block that allows selection of these rates.

| Rate | J23 row (counter output) | J24 row (counter output) |
|---|---|---|
| (top row) | J21 | J22 |
| 500K | 5 | 5 |
| 250K | 2 | 2 |
| 125K | 1 | 1 |
| 62.5K | 6 | 6 |
| 31.25K | 3 | 3 |

For relative user 2, jumper the proper rate across on J23. For relative user 3, jumper the
proper rate across on J24.

## Universal Parallel Channel (pages 22-24)

### Technical Overview (page 22)

The UNIVERSAL PARALLEL section of the INTERFACER 4 consists of a full duplex latched parallel
port for I/O data and one port for status. The use of TTL latches rather than a MOS parallel
interface chip eliminates the need for mode selection and initialization, and allows the port to
have strobe, attention and enable bits, an input interrupt, and 16 true data lines.

### I/O Address Assignment (page 22)

The UNIVERSAL PARALLEL channel on the INTERFACER 4 board is addressed as the MODE and CONTROL
registers of RELATIVE USER 0. The DATA-P register of the channel is addressed at the PORT BASE +
2 (USART equivalent is the MODE register), and the STATUS-P register is addressed at PORT BASE +
3 (USART equivalent is the CONTROL register). In the STATUS-P register only data bits 0 and 1
are significant.

### Status-P Register Bit Assignment (page 22)

Inputs to the processor from the STATUS-P register are defined as follows:

| Data bit | Name | Signal |
|---|---|---|
| D0 | DAV0 | DATA AVAILABLE CHANNEL |
| D1 | TKN0 | DATA TAKEN CHANNEL |
| D2-D7 | NOT USED | |

### Port Control Lines - Input Strobe Line (pages 22-23)

The STROBE line on the input Channel is used to latch the data into the input register when a
74LS374 or 74LS373 latch is used. This line also sets the status flag so that the processor can
tell if data has been entered.

If a 74LS374 is used as the input register, a transition on the strobe line latches the data and
sets the status flag. The strobe polarity select switch should be set as described below so that
your data is valid during the transition. With the select switch (S1-2) ON, a low to high
transition on strobe will latch the data. With the select switch OFF, a high to low transition
on strobe will latch the data.

If a 74LS373 is used as the input register, the strobe line can assume two different modes. The
first mode is similar to the latched mode of the 74LS374 described above except that during the
strobe pulse the data is transparent through the latch to the processor. At the end of the
strobe pulse, the data will be latched and stable for the processor to access. With the strobe
select switch (S1-2) "ON", a positive going strobe pulse will latch the data at the end of the
pulse. With the select switch "OFF", a negative going strobe pulse will latch the data at the
end of the pulse. The second mode is the fully transparent mode where the data is never latched
but is available for inputting at any time by the processor. This mode is useful whenever the
data has no strobe bit associated with it. This mode is entered when the strobe line is left
open with the strobe select switch "ON". See the table below for strobing data.

Page 23 timing drawings, for the strobe select switch "ON" (a positive strobe pulse) and "OFF"
(a negative pulse): with a 74LS374, the data changes from OLD DATA to NEW DATA at the leading
edge; with a 74LS373, the data is TRANSPARENT during the pulse and LATCHED DATA after it.

### Input Examples (page 23)

Some examples of typical applications might include connecting a ASCII keyboard or a set of
sense switches to the input Channel of the INTERFACER 4. A keyboard usually has a strobe line to
indicate that it has current valid data on its lines. Therefore, using one of the 74LS374
latches would be best. The keyboard data lines would be connected accordingly to input data
lines, and the strobe line would be connected, and the strobe select switch would be "ON" for a
positive keyboard strobe, and "OFF" for a negative keyboard strobe. If connecting some sense
switches to the input lines, a 74LS373 would be the best choice because there are usually no
strobe lines associated with switches. The switches should be connected to the input lines so
that they ground the inputs (no pullup resistors are needed since they are supplied on the
board) and the STROBE LINE should be left floating with the strobe select switch "ON". This
allows the processor to input the data from the switches at any time.

### Output Enable Line (page 23)

The OUTPUT ENABLE LINE on the Channel serves two functions depending on the user's
configuration. In handshaking operations, it is used to enable the output of the DATA-P register
which is normally tri-stated. This line also resets the ATTENTION bit and informs the processor,
through the status port, that the data has been taken from the latch. In strobed operations, the
OUTPUT ENABLE LINE is used to enable the output of the register at all times. When the select
switch (S1-1) is "ON", the OUTPUT ENABLE LINE must be low to enable the outputs. With the select
switch "OFF", the OUTPUT ENABLE LINE must be high to enable the output.

### Attention Line (pages 23-24)

The ATTENTION LINE is used to inform an external device that new data is now available for it.
This line may be jumpered (J1) to provide any one of four different outputs. With the Common
(top row of four pins) jumpered to either Q or Q\*, and the OUTPUT ENABLE LINE set so that the
output of the register is Tri-Stated, then the ATTENTION LINE will go high (Q) or low (Q\*) when
data is strobed into the output register. When the OUTPUT ENABLE LINE level is changed to enable
the data, then the ATTENTION LINE will return to its original level. In this mode, the OUTPUT
ENABLE LINE is used to transfer the data out of the register and reset the attention flag.
Since the level of the ATTENTION LINE may be sampled by the processor through the status port, a
high speed handshaking data transfer can occur.

With the Common tied to either the "P" or the "P\*", the ATTENTION LINE becomes a positive (P) or
negative (P\*) going strobe pulse with a width of the system pWR\* strobe (between 150 and
1000ns ). In this mode, the state of the OUTPUT ENABLE LINE should be set so that the data is
enabled at all times. This mode is best used when the external device needs the data strobed
into it. See the diagrams below for the output data timing using either a 74LS373 or a 74LS374
as an output register.

OUTPUT DATA TIMING (drawing): Q rises and Q\* falls when data is written, and both return when
the OUTPUT ENABLE LINE changes; the DATA goes from TRI-STATED to ENABLED at that change. The
pulse outputs (the drawing labels them L and R) give one pulse during the write. With a
74LS373 the data is transparent during the write (TRANS.) and then NEW DATA; with a 74LS374 it
changes to NEW DATA at once.

### Output Examples (page 24)

Some examples of typical applications might include connecting an A to D converter or some LED's
to the output lines. An A to D converter will probably require 8 data lines be connected in
addition to a strobe line. In many cases, the strobe (P or P\*) connected to the ATTENTION LINE
will be sufficiently long for the converter and can be connected directly with the proper
polarity for correct operation. If only 7 data lines are required for the converter, then the
eighth data line may be used.

## Centronics Style Parallel Channel (pages 25-28)

### Technical Overview (page 25)

The CENTRONICS PARALLEL section of the INTERFACER 4 consists of an 8 bit latched output port for
data and a full complement of status and control lines. The output STROBE line timing conforms
with the timing specifications of all known CENTRONICS interface printers, and power-up
programming of the control lines allows flexible initialization procedures.

### I/O Address Assignment (page 25)

The CENTRONICS PARALLEL channel on the INTERFACER 4 board is addressed as the DATA and STATUS
registers of RELATIVE USER 0. The DATA-C register of the channel is addressed at the PORT BASE +
0 and the STATUS-C / CONTROL-C register is addressed at PORT BASE + 1.

### Status-C Register Bit Assignment (page 25)

Inputs to the processor from the STATUS-C register are defined as follows:

| Data bit | Name | Signal |
|---|---|---|
| D0 | PBMT | PRINTER BUFFER EMPTY - READY FOR CHARACTER WHEN HIGH |
| D1 | NOT USED | ALWAYS LOGIC "0" |
| D2 | ACKNLG | TRANSFER ACKNOWLEDGE - 10uS LOW PULSE |
| D3 | PE | PAPER ERROR - PRINTER OUT OF PAPER WHEN HIGH |
| D4 | ERROR | PRINTER ERROR WHEN HIGH |
| D5 | ERROR | PRINTER ERROR WHEN HIGH |
| D6 | SLCT | PRINTER SELECTED "ON" WHEN HIGH |
| D7 | BUSY\* | PRINTER BUSY WHEN LOW |

The status register bit assignment was designed to minimize the amount of software alteration
required to use a parallel printer. As configured, the status word should be compatible with
most currently available BIOS routines. With this channel selected as EXACT USER 6, it is 100%
compatible with standard CompuPro software.

### Control-C Register Bit Assignments (page 25)

Outputs to the CONTROL-C register from the processor are defined as follows:

| Data bit | Name | Signal |
|---|---|---|
| D0 | AFX | AUTO FEED EXTERNAL - AUTO LINE FEED AFTER RETURN |
| D1 | INIT | INITIALIZE - INITIALIZE PRINTER |
| D2 | SLCTIN | SELECT INPUT - PRINTER SELECT INPUT |
| D3 | LED | LIGHT EMITTING DIODE - HIGH = ON |
| D4-D7 | NC | NOT USED |

### Centronics Transmit Interrupt (page 26)

The INTERFACER 4 comes configured to generate an interrupt upon receiving an ACKNOWLEDGE pulse
from the printer indicating that it is ready to accept another byte of data. Provisions have
been made to generate an interrupt upon the printer's change from BUSY to NOT BUSY. This may be
accomplished by cutting the shorting trace at jumper J5 TOP, and installing a shorting plug on J5
BOTTOM.

### Centronics Interface Signal Designations (page 26)

The following table describes the CENTRONICS cable pinout designations as defined by EPSON.
There exist several minor differences between the CENTRONICS signal designations and those of
EPSON. The EPSON designations are shown here because they are somewhat more complete. All
differences will be marked with a "\*" and explained below.

| Signal pin | Ground pin | Signal name | Signal description | |
|---|---|---|---|---|
| 1 | 19 | STROBE\* | ACTIVE LOW DATA STROBE PULSE | |
| 2 | 20 | DATA 1 | DATA BIT 1 | |
| 3 | 21 | DATA 2 | DATA BIT 2 | |
| 4 | 22 | DATA 3 | DATA BIT 3 | |
| 5 | 23 | DATA 4 | DATA BIT 4 | |
| 6 | 24 | DATA 5 | DATA BIT 5 | |
| 7 | 25 | DATA 6 | DATA BIT 6 | |
| 8 | 26 | DATA 7 | DATA BIT 7 | |
| 9 | 27 | DATA 8 | DATA BIT 8 | |
| 10 | 28 | ACKNLG\* | ACKNOWLEDGE PULSE ACTIVE LOW | |
| 11 | 29 | BUSY | PRINTER BUSY ACTIVE HIGH | |
| 12 | 30 | PE | PAPER ERROR ACTIVE HIGH | |
| 13 | -- | SLCT | PRINTER "ON" WHEN HIGH | |
| 14 | -- | AUTO FEED XT\* | AUTO LINE FEED WHEN LOW | \* |
| 15 | -- | NC | NOT USED | \* |
| 16 | -- | 0V | LOGIC GROUND LEVEL | |
| 17 | -- | CHASGND | PRINTER CHASSIS GROUND | |
| 18 | -- | NC | NOT USED | \* |
| 19-30 | -- | GND | GROUND RETURN LINES | |
| 31 | -- | INIT\* | PRINTER INITIALIZE WHEN LOW > 50 uS | |
| 32 | -- | ERROR\* | PRINTER ERROR WHEN LOW | |
| 33 | -- | GND | GROUND | |
| 34 | -- | NC | NOT USED | \* |
| 35 | -- | NC | NOT USED | \* |
| 36 | -- | SLCT IN\* | PRINTER SELECT INPUT WHEN LOW | \* |

### Signal Differences between EPSON and CENTRONICS (pages 26-27)

| Line | Difference |
|---|---|
| 14 | This line is defined as 0V or signal ground by CENTRONICS. J2 should be removed when used with a CENTRONICS Printer. |
| 15 | This line is designated as OSCXT by CENTRONICS. This is not a conflict. |
| 18 | This line is designated as +5V by CENTRONICS. This is not a conflict. |
| 34 | This line is designated LINE COUNT PULSE by CENTRONICS. This is not a conflict. |
| 35 | This line is designated as a ground return by CENTRONICS. This is not a conflict. |
| 36 | This line is not used by CENTRONICS, however, J3 should be removed when used with a CENTRONICS Printer. |

NOTE: The user should always refer to the interface specifications of his printer before
connecting it to the INTERFACER 4.

### Centronics Control-C Line Jumpering (page 27)

The printer control lines handled by the CONTROL-C register may be set to power-up either high
or low by the proper setting of jumpers J2-J4. This allows the user to select the power-up
condition of the printer independent of the initialization procedure used. Jumper J5 is not a
CONTROL-C jumper, but controls whether the interrupt is generated on ACKNLG\* or BUSY. It is
included in this section because it is located in the same block of jumpers. The following
section describes the position of the jumpers and their effects.

Jumpers J2-J5 are located in between U13 and U14, and above LED1, and are arranged in the
following format (a drawing of eight two-pin positions, from top to bottom): J2 TOP, J2 BOTTOM,
J3 TOP, J3 BOTTOM, J4 TOP, J4 BOTTOM, J5 TOP, J5 BOTTOM.

### Jumper Description (pages 27-28)

| Jumper | Signal | Description |
|---|---|---|
| J2 | AUTO FEED XT\* | This signal controls whether the printer does an automatic line feed after receiving a "return". J2 TOP: Low on power-up when jumpered. J2 BOTTOM: High on power-up when jumpered. |
| J3 | SLCT IN\* | This signal enables the printer to receive data when Low. Printer disabled when High. J3 TOP: Low on power-up when jumpered. J3 BOTTOM: High on power-up when jumpered. |
| J4 | INIT\* | This signal initializes the printer controller when low for more than 50 uS. High normally. J4 TOP: Low on power-up when jumpered. J4 BOTTOM: High on power-up when jumpered. |
| J5 | INTERRUPT | This jumper determines whether the CENTRONICS interrupt is generated from ACKNLG\* or BUSY. J5 TOP: Interrupt on ACKNLG\* - shorted as shipped on board. J5 BOTTOM: Interrupt on BUSY - MUST CUT TOP SHORT! |

### Light Emitting Diode (page 28)

LIGHT EMITTING DIODE LED1 is controlled by the CONTROL-C register bit D3, and may be turned "ON"
or "OFF" when outputting to this port. The LED will always be "OFF" upon power-up or RESET, and
may be turned "ON" by outputting a logic "1" to D3.

### Sense Dip Switch (page 28)

By reading the DATA-C register, the state of DIP switch S1, positions 3-10 may be determined
under program control. When read, an "ON" position will read as a "0", and an "OFF" position will
be read as a "1". Positions 3 thru 10 correspond to DATA bits 7 thru 0 when read.

EXAMPLE: If a 0Fh is read, positions 3-6 are "ON", and positions 7-10 are "OFF".

NOTE: Maximum allowable length for the Centronics cable from enclosure to printer is six feet.

## Theory of Operation (pages 29-32)

The INTERFACER 4 can be roughly divided into 9 subsections for describing its operation. These
sections include: The S-100 Bus Drivers, the I/O Port Decode Logic, the Strobe Generation Logic,
the Wait State Logic, the Interrupt Control/Status Logic, the USART, the RS-232/CURRENT LOOP
Level Conversion Logic, The CENTRONICS Parallel Logic, and the Universal Parallel Logic.

### S-100 Bus Drivers (page 29)

The separate data input and output data buses of the S-100 bus are converted to a
bi-directional data bus by octal drivers U45 and U46. Data from the S-100 bus is driven onto the
internal data bus by U45 only when sOUT goes high, indicating an output operation. The internal
data bus is driven onto the S-100 bus either as a high or low nibble, or as a full byte. When
DOEN\* goes low, indicating that valid board select (SEL) and pDBIN are high (NAND-U40), and A2
is low, both DOENL\* and DOENH\* go low and enable a full byte onto the bus. When A2 is high,
either the high or low nibble is gated out onto the bus depending on the state of HSEL (U5,
U24). This allows the interrupt status to be read from 2 boards at the same time, each suppling
the proper nibble of data.

All S-100 bus signals are buffered onto the board if the line would otherwise have more than 1
LSTTL load. Address lines A0, A1, A2, and pDBIN are buffered onto the board by 2/3 of hex buffer
U43, and the lines sOUT, sINP, pWR\*, 0, and pSTVAL\* are inverted using portions of U42

### I/O Port Decode Logic (pages 29-30)

The eight port block that the INTERFACER 4 occupies is decoded by 6 open collector X-OR gates
(U22 and U41). 5 of these gates decode address lines A3-A7 by comparing against positions 5-9 of
switch S2, and the last section compares sOUT and sINP\* to determine if an I/O operation is
occurring. When all compare conditions are satisfied, ASEL goes high. Closing position 4 of S2
will ground ASEL and disable the board completely.

A valid board select (SEL\*) is generated (by 1/3 of U23), when ASEL goes high along with USEL
(indicating that this boards select number is active) and A1 and A2 are not both high
(indicating the USER SELECT PORT is not selected). SEL\* is disabled by 1/3 of U23 when the USER
SELECT PORT is enabled so that conflicts between up to eight boards do not occur.

A USER SELECT write occurs when ASEL, A1, A2, sOUT, and STROBE go high. This generates OUT7\*
(U23) which clocks the least significant 5 bits on the bus (D0-D4) into hex latch U18. The 2 low
order bits of U18 are decoded into 4 chip enables (CE0\* - CE3\*) by U25 when SEL is high, A2 and
ESTROBE\* are low, and SH/L\* is low. Bit D2 (H/L\*), is either buffered or inverted by X-OR U5
and S2-3. This signal is low if the board is selected and also indicates that a high or low
nibble is to be read. The 2 high order bits of U18 are compared to switch positions 1 and 2 of S2
by 1/2 of U22 (X-NOR) to decode a current user board select signal USEL. Access to registers on
the board requires that USEL be high before access is gained.

The four interrupt read and write strobes are generated by decoder U44 when A2 is high and SEL\*
and STROBE\* are low. A0, A1, and sINP\* determine which output becomes active at the proper
time.

### Strobe Generation Logic (page 30)

In order to gain additional access time in an I/O cycle for the 2651 USARTs, the INTERFACER 4
generates early strobes based on valid status. S-100 bus strobes pDBIN and pWR\* are gated
together (U19) and inverted to generate STROBE and STROBE\*. These signals indicate that a bus
strobe is occurring. The interrupt registers and user select port have their data gated by
STROBE because they are TTL and capable of very high speed operation. Since the 2651 type USART
is a MOS device and has an access time of approximately 250 nS, an early strobe is generated so
that wait states are avoided whenever possible. A status valid signal, ESTATVAL\*, is generated
whenever pSYNC is high and pSTVAL\* is low. ESTATVAL\* clears "D" flop U16a to generate
ESTROBE\*, which becomes one term of the USART chip enable decoder U25. The termination of
STROBE\* causes a "1" to be clocked into U16a and terminate ESTROBE\*.

### Wait State Logic (page 30)

To allow operation with high speed processors, a wait state generator allows the addition of 1,
2, or 3 wait cycles. U20a and U21 forms a 3 bit shift register clocked by Φ\*. A wait state is
left pending after STROBE goes low, and when STALL1\*, STALL2\* or STALL3\* and A2 are low (U19),
and SEL is high (U40), WAIT\* is generated. STALL1\* is clocked out on the next rising edge of
Φ\* after STROBE goes high, STALL2\* is clocked out the following cycle and STALL3\* is clocked
out on the 3rd cycle. The pRDY\* line is pulled low by U43 when WAIT\* goes low. When neither J6,
J7, OR J8 is connected, no wait states will be generated.

### Interrupt Control/Status Logic (pages 30-31)

The interrupt logic consists of two 4 bit latches for enabling interrupts onto the bus, two 4
bit buffers for reading current interrupt status, and eight 2 input open collector NAND buffers
for driving the interrupts on the bus.

Two 4 bit latches (U48, U52) are used for generating the interrupt enable mask. The Q outputs
become the RxINTENx and TxINTENx interrupt enables for selectively masking "OFF" individual
interrupts. Upon power-up or reset, these latches are cleared by CLR\* so that all interrupts are
disabled.

The TxRDY and RxRDY interrupt outputs from the 2651 USARTs are inverted to form active high
interrupt signals. The CENTRONICS Parallel and Universal Parallel channels generate active high
interrupts automatically. These interrupt signals are fed to one input of the open collector NAND
buffer (U47, U51), with the corresponding interrupt enable fed to the other input. The resulting
interrupt outputs (TxINTx and RxINTx) are capable of driving the VI0-7 lines directly, and are
brought out to JS5 and JS6 for jumpering to the appropriate line.

Two 4 bit buffers are formed from two quad tri-state buffers (U49, U53) for gating the current
USART and parallel interrupts (TxRDYx and RxRDYx) onto the bus as status information. Since the
buffers use Tx and Rx RDY instead of Tx and Rx INT lines, the status of disabled as well as
enabled interrupts are displayed.

Relative channels 1, 2, and 3 allow jumpering the TxEMT/DSCHG interrupt from the USART to either
the TxRDY or RxRDY interrupt outputs. This is possible since the outputs from the 2651 are open
drain and may be wire-ORed.

### USARTS (page 31)

The 2651 type USART is quite sophisticated in that it can run in both asynchronous as well as
synchronous modes. In addition, the part has an internal baud rate generator, RS-232 status and
control bits, up to 3 interrupt outputs, and the capability of transmitting as well as receiving
baud clocks.

The chip enable (CE) and read/write (R\*/W) lines are operated by initially determining whether
a read or a write will occur (sINP\* to R\*/W) and then strobing the part with CE\*. Address lines
A0 and A1 determine which of four registers will be selected and CLR resets the USART.

The baud rate clock BAUDCLK is generated by a 5.0688 MHz crystal oscillator formed from 3
inverters (U54) and crystal X1.

### RS-232/Current Loop Level Conversion Logic (page 31)

Each USART has a full complement of RS-232 handshaking lines for devices that require them.
Industry standard 1488 and 1489 receivers and transmitters are used throughout for highest
performance. In addition to the data lines TxD and RxD, each channel has a RTS and DTR output and
a CTS, DSR, and DCD input. All three RS-232 status lines have pullup resistors to +12V so that
floating inputs are pulled high.

Relative Users 2 and 3 are capable of sending and receiving both the transmit and receive baud
clocks for running in synchronous mode. An RS-232 driver and a receiver are provided for RxC and
TxC, and either one may be jumpered in. In addition, a dual 4 bit counter (U55) is used to divide
the 2 MHz bus clock down to 31.25 KHz to 500 KHz for running the USARTs faster than their
internal baud rate generators provide. Flexible jumpering allows either or both channels to run
at the higher rates.

Relative user 1 may be set to run in current loop mode by appropriately jumpering JS4. Optical
isolators U29 and U30 are used if isolation is required. R4 provides the current source for U29,
whose output is inverted (U54), and then converted to RS-232 by the free section of U33. This
output may be jumpered to pin 2 of JS3 by J25. Transmit data (TxD) is inverted (U54), isolated
(U30), and buffered by Q1 for which R8 is the current source.

### CENTRONICS Parallel Logic (pages 31-32)

The CENTRONICS Parallel logic consists of an octal data latch, a quad control latch, an octal
status buffer, an octal status buffer with a DIP switch,
a strobe one-shot, and a control strobe decoder. Decoder (U15) generates eight separate control
strobes for both the CENTRONICS parallel logic and the Universal parallel logic. Depending on A0,
A1, and sINP\*, the 8 strobes are generated when STROBE\* and CPE\* are low. The output data
register (U11) is clocked by inverted DWR\* and latches 8 data bits off the internal data bus.
The data strobe is provided by dual one-shot U10, which when strobed by DWR\*, generates a 1 uS
delay and then a 1 uS data strobe to accommodate all known data set-up and strobe length times.
Printer status is gated onto the internal data bus by octal buffer U12 when SRD\* strobes low.
The status word is arranged similarly to the status register of the 2651 USART to facilitate
software compatibility. Quad control register U13 latches the 4 low order data bits off the
internal data bus when SWR\* strobes low, and is cleared on reset for a known power-up state.
Jumpers J2, J3, and J4 allow either Q or Q\* to control the printers AUTO FEED XT\*, INIT\*, and
SLCT IN\* lines for any power-up state that can be altered under software control. The LED is
controlled by bit D3 and Q\* so that it is off after reset. Sense DIP switch S1 positions 3-10
are buffered (U14) onto the internal data bus when DRD\* is strobed low. The CENTRONICS
interrupt TxI0 is generated at the end of an ACKNLG\* pulse from the printer, and is cleared when
new data is written to the data register (DWR\*). Jumper J5 allows the interrupt to be generated
by BUSY if required.

### Universal Parallel Logic (page 32)

The Universal Parallel logic consists of 2 octal data registers, 2 "D" type flip flops, 3 X-OR
gates and 2 status buffers. Output data is latched from the internal data bus by U7 when MWR\*
strobes low. MWR\* also sets flop U8b which with J1 provides the attention level or pulse, and
the DNTKN status bit 1 flag. When the data register is brought active by ENABLE through U5, the
attention level and DNTKN flag are cleared. Input data is latched into U6 when STROBE is pulsed
through U5. This also clocks a "1" into U8b, which generates the RxI0 interrupt, and sets the DAV
status bit 0 flag. Input data is gated onto the internal data bus when MRD\* strobes low, and the
interrupt and DAV status is cleared.

**[sic]** U8b is named for both the output (attention/DNTKN) flop and the input (DAV/RxI0) flop.
The text says the logic has two "D" flip flops; which one is U8a is not stated.

## Software Section (pages 33-40)

### Sample Program for Using the Interfacer 4 as the CP/M Console (pages 33-35)

```
;       CompuPro INTERFACER 4 support routines

GBI3:   EQU     10h             ;INTERFACER 4 Base address
GBI3D:  EQU     GBI3+0          ;Uart data location
GBI3S:  EQU     GBI3+1          ;Uart status
GBI3M:  EQU     GBI3+2          ;Uart mode register
GBI3C:  EQU     GBI3+3          ;Uart command register
GBI3U:  EQU     GBI3+7          ;Uart select register

GBI3DV: EQU     00000010b       ;INTERFACER 4 Data Available
GBI3MT: EQU     00000001b       ;INTERFACER 4 Transmit Buffer Empty
GBI3DS: EQU     10000000b       ;INTERFACER 4 Data Set Ready
CON:    EQU     7               ;INTERFACER 4 Console Select
PRN:    EQU     6               ;INTERFACER 4 Printer Select
ULS:    EQU     5               ;INTERFACER 4 UL1 Select

;       C O N S O L E   I N I T I A L I Z A T I O N
;
;       This routine performs the initialization required by the INTERFACER 4.
;

I3INIT:
        MVI     A,CON           ;Console select
        OUT     GBI3U           ;Select Uart 7
        MVI     A,11101110b     ;Async, 16x, 8 bits, no parity, even, 2 stops
        OUT     GBI3M           ;Set up mode register 1
        MVI     A,01111110b     ;9600 baud
        OUT     GBI3M           ;Set up mode register 2
        MVI     A,00100111b     ;Trans. on, dtr low, rec. on, no break,
                                ;       no reset, rts low
        OUT     GBI3C           ;Set up command port

        MVI     A,PRN           ;Printer Select
        OUT     GBI3U           ;Select Uart 0
        MVI     A,11101110b     ;Async, 16x, 8 bits, no parity, even, 2 stops
        OUT     GBI3M           ;Set up mode register 1
        MVI     A,01111110b     ;9600 baud
        OUT     GBI3M           ;Set up mode register 2
        MVI     A,00100111b     ;Trans. on, dtr low, rec. on, no break,
                                ;       no reset, rts low
        OUT     GBI3C           ;Set up command port

        MVI     A,ULS           ;User list 1 Select
        OUT     GBI3U           ;Select Uart 0
        MVI     A,11101110b     ;Async, 16x, 8 bits, no parity, even, 2 stops
        OUT     GBI3M           ;Set up mode register 1
        MVI     A,01111110b     ;9600 baud
        OUT     GBI3M           ;Set up mode register 2
        MVI     A,00100111b     ;Trans. on, dtr low, rec. on, no break,
                                ;       no reset, rts low
        OUT     GBI3C           ;Set up command port
        RET

;       C O N S O L E   S T A T U S
;
;       This routine samples the Console status and returns the following
;       values in the A register.
;
;       EXIT    A = 0 (zero), means no character
;               currently ready to read.
;
;               A = FFh (255), means character
;               currently ready to read.

I3CONST:
        MVI     A,CON
        OUT     GBI3U
        IN      GBI3S           ;Input from port
        ANI     GBI3DV          ;Mask data available
        RZ                      ;If data not available
        ORI     0FFH
        RET

;       C O N S O L E   I N P U T
;
;       Read the next character into the A register, clearing the high
;       order bit. If no character currently ready to read then wait for a
;       character to arrive before returning.
;
;       EXIT    A = character read from terminal.

I3CONIN:
        MVI     A,CON
        OUT     GBI3U
        IN      GBI3S           ;Get status from uart
        ANI     GBI3DV
        JZ      I3CONIN
        IN      GBI3D
        ANI     7Fh
        RET

;       C O N S O L E   O U T P U T
;
;       Send a character to the console. If the console is not ready to
;       receive a character wait until the console is ready.
;
;       ENTRY   C = ASCII character to output to console.

I3CONOUT:
        MVI     A,CON
        OUT     GBI3U
        IN      GBI3S           ;Get uart status
        ANI     GBI3MT          ;Test if buffer empty
        JZ      I3CONOUT
        MOV     A,C
        OUT     GBI3D
        RET
;
;       L i s t   O u t p u t.
;
;       Send a character to the list device. If the list device is not ready
;       to receive a character wait until the device is ready.
;
;       ENTRY   C = ASCII character to be output.

I3LIST: LDA     IOBYTE          ;Get IOBYTE status
        ANI     0C0H            ;Check for UL1:
        SUI     0C0H
        MVI     A,ULS
        JZ      I3UL1
        MVI     A,PRN
I3UL1:  OUT     GBI3U
I3LST1: IN      GBI3S
        ANI     GBI3MT+GBI3DS
        SUI     GBI3MT+GBI3DS
        JNZ     I3LST1
        MOV     A,C
        OUT     GBI3D
        RET
;
;       L i s t   S t a t u s.
;
;       Return the ready status for the list device.
;
;       EXIT    A = 0 (zero), list device is not ready to
;               accept another character.
;               A = FFh (255), list device is ready to accept
;               a character.

I3LST:  LDA     IOBYTE
        ANI     0C0H            ;Check for UL1:
        SUI     0C0H
        MVI     A,ULS
        JZ      I3LS1
        MVI     A,PRN
I3LS1:  OUT     GBI3U
        IN      GBI3S
        ANI     GBI3MT+GBI3DS
        SUI     GBI3MT+GBI3DS
        MVI     A,0FFH
        RZ
        XRA     A
        RET
```

Notes on the listing (not in the manual):

- The comments "Select Uart 0" on the PRN and ULS blocks are left from a copy; the code selects
  users 6 and 5. The mode 1 comment "no parity, even" describes 11101110b: parity disabled, so the
  "even" bit has no effect.
- The list routines wait for TxRDY **and** DSR (`GBI3MT+GBI3DS`). With the Centronics channel as
  user 6 (J26 A-C/B-D), the same test reads Status-C: D0 PBMT and D7 BUSY\*. So the routine works
  unchanged on a serial printer or a Centronics printer. That is the "100% compatible" claim of
  page 25.

### Centronics Test Program (pages 36-38)

```
; 3/25/82

;*              CENTRONICS TEST PROGRAM
;*
;*   This program will output all standard ASCII characters to the printer
;*   along with the EPSON graphics characters controlled by bit 8. The
;*   program will stop when any key is hit on the console. The printer
;*   is required to be USER 4 at ports 10-17. J2, J3, and J4 should be on
;*   the top pair of pins with an EPSON. J2 and J3 should be removed
;*   entirely with a CENTRONICS printer.
;*
base    equ     10h
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
cr      equ     0dh     ;carr. return
lf      equ     0ah     ;line feed
*
*
*
        org     100h
start1  call    setup   ;setup message area
start2  call    start   ;init user
line1   lxi     h,msg1  ;point to message
        call    print   ;print message
        lxi     h,msg3  ;point
        call    print   ;print
        lxi     h,msg4  ;point
        call    print   ;print
        lxi     h,msg5  ;point graphics
        call    print   ;print graphics
        mvi     c,0bh   ;check
        call    0005h   ;  console
        cpi     00h     ;    status
        jz      start2  ;for entry
        jmp     0       ;exit

Start   mvi     a,04H   ;init CENTRONICS
        out     user    ;select uart
        mvi     a,0ffh  ;interrupts enable
        out     txreg   ;transmit int enabled
        out     rxreg   ;receive enabled
        mvi     a,0bh   ;init centronics
        out     ustat   ;out
        ret

setup   lxi     h,msg3  ;point to buffer
        mvi     a,20h   ;init
sloop   mov     m,a     ;put byte
        inr     a       ;next up
        inx     h       ;next loc
        cpi     40h     ;beyond last char
        jz      done1   ;
        jmp     sloop   ;again
done1   mvi     a,cr    ;carrage return
        mov     m,a     ;
        inx     h       ;
        mvi     a,lf    ;line feed
        mov     m,a     ;
        inx     h       ;
        mvi     a,0     ;null
        mov     m,a     ;place
setup1  lxi     h,msg4  ;point to buffer
        mvi     a,40h   ;init
sloop1  mov     m,a     ;put byte
        inr     a       ;next up
        inx     h       ;next loc
        cpi     7eh     ;beyond last char
        jz      done2   ;
        jmp     sloop1  ;again
done2   mvi     a,cr    ;carrage return
        mov     m,a     ;
        inx     h       ;
        mvi     a,lf    ;line feed
        mov     m,a     ;
        inx     h       ;
        mvi     a,0     ;null
        mov     m,a     ;place
setup2  lxi     h,msg5  ;point to buffer
        mvi     a,0A0h  ;init
sloop2  mov     m,a     ;put byte
        inr     a       ;next up
        inx     h       ;next loc
        cpi     0E0h    ;beyond last char
        jz      done3   ;
        jmp     sloop2  ;again
done3   mvi     a,cr    ;carrage return
        mov     m,a     ;
        inx     h       ;
        mvi     a,lf    ;line feed
        mov     m,a     ;
        inx     h       ;
        mvi     a,0     ;null
        mov     m,a     ;place
        ret
print   call    start   ;select user
        call    instat  ;check status
        mov     a,m     ;get byte
        cpi     0       ;is it a null?
        rz              ;done
        out     udata   ;output data
        inx     h       ;next byte
        jmp     print   ;again

instat  in      ustat   ;get status
        cpi     0c5h    ;check all ok?
        jnz     instat  ;loop not ready
        ret
msg1    db      cr,lf,lf
        db      'This is a test of the INTERFACER 4 CENTRONICS port'
        db      cr,lf,lf
        db      0
msg2    db      cr,lf,lf
        db      0
msg3    ds      100h
msg4    ds      100h
msg5    ds      100h
        end
```

Notes on the listing (not in the manual):

- `Start` writes 0BH to CONTROL-C: AFX, INIT and LED on, SLCTIN off.
- `instat` waits for Status-C = 0C5H: BUSY\* high (not busy), SLCT, ACKNLG high (no pulse) and
  PBMT. So the ACKNLG bit reads 1 between pulses, and D1 reads 0 as page 25 says.
- `print` calls `Start` again for every byte, so it rewrites the masks and CONTROL-C each time.

### Interfacer 4 Serial Test Program (pages 39-40)

```
;*              INTERFACER 4 SERIAL TEST PROGRAM
;*
;*      This program will initialize 2651s for asynchronous operation at
;*      9600 baud with 8 data bits, one stop bit, no parity. If the sense
;*      switch position 10 is "ON", RELATIVE USER 1 will run at 110 baud for
;*      current loop testing. This program will echo all characters
;*      received on any user channel (from 0 to 31 except the CENTRONICS
;*      PORT) and if any user sends a ^C, the program will terminate and
;*      return back to CP/M.
;*
base    equ     10h
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
fs      mvi     a,0     ;first board
        out     user    ;select
        mvi     a,0ffh  ;interrupts on
        out     txreg   ;enable transmit int
        out     rxreg   ;enable receive int
Start   mvi     a,0ffh  ;init user
Loop    inr     a       ;next user
        cpi     20H     ;check for final uart
        jz      echo    ;start echo routine
usel    out     user    ;select uart
        mov     b,a     ;save user in b
        ani     3       ;mask for centronics
        cz      cinit   ;sense I-loop
        call    init    ;init the uart
        mov     a,b     ;restore user
        jmp     loop    ;next
Cinit   in      udata   ;get sense switch
        ani     1       ;bit 0
        jz      iloop   ;110 baud
        mvi     e,7Eh   ;9600 baud
nu      mov     a,b     ;resore user
        inr     a       ;next user
        mov     b,a     ;save user
        out     user    ;select next user
        ret             ;init usart 9600
iloop   mvi     e,72h   ;110 baud
        jmp     nu      ;next user
Init    mvi     a,0CEh  ;set up the 2651
        out     mode    ;send to mode register 1
        mov     a,e     ;get baud value
        out     mode    ;SEND BYTE TO M.R. 2
        mvi     a,27h   ;could be 07h (no 1420)
        out     commr
        mvi     e,7eh   ;reset 9600
        ret
Echo    mvi     a,0FFh  ;mask value
        out     txreg   ;set tx int reg
        out     rxreg   ;set rx int reg
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
        mov     a,e     ;save input in E reg.
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

Notes on the listing (not in the manual):

- **[sic]** `Inloop` has `mov a,e ;save input in E reg.`; `mov e,a` is meant. As printed, the
  echo sends the previous E (7EH after `Init`).
- At each relative user 0 (`ani 3`, `cz cinit`) the program reads DATA-C, the sense switch: bit 0
  is S1 position 10. It then moves to relative user 1 and gives it 110 baud if position 10 is ON.
- The program writes the masks (0FFH) for board 0 only, before it selects any other user.

## INS2651 Programmable Communications Interface (pages 41-47)

Page 41 is a title page. Pages 42-47 are the National Semiconductor INS2651 data sheet, dated
October 1980, printed sideways. It is the same data sheet as pages 25-30 of the Interfacer 3
manual; see `interfacer-3.md`, section "INS2651 Programmable Communications Interface", for the
facts that matter to the board. It is not transcribed again. The pages hold:

| Page | Contents |
|---|---|
| 42 | General description, features, system configuration diagram; absolute maximum ratings, DC characteristics, capacitance |
| 43 | AC characteristics (bus parameters, other timings); timing waveforms (reset, read/write, clock, transmit, receive) |
| 44 | Timing waveforms, continued (synchronous and asynchronous mode); block diagram; functional pin definitions, input signals |
| 45 | Output signals; input/output signals; pin configuration; programming (Table 1, register addressing); Figure 1, initialization flowchart |
| 46 | Mode Register 1 and 2, Command and Status register formats; Table 2, baud rate generator characteristics |
| 47 | Operation (general, synchronous mode, asynchronous mode); physical dimensions; sales offices |

## Hardware Description (pages 48-58)

### Parts List (page 48)

| Ref | Part | Ref | Part |
|---|---|---|---|
| U1 | 7912 | U46 | 74LS244 |
| U2 | 7812 | U47 | 74LS38 |
| U3-U4 | 7805 | U48 | 74LS175 |
| U5 | 74LS386 | U49 | 74LS125 |
| U6 | 74LS373/374 | U50 | 74LS04 |
| U7 | 74LS373/374 | U51 | 74LS38 |
| U8 | 74LS74 | U52 | 74LS175 |
| U9 | 74LS125 | U53 | 74LS125 |
| U10 | 74LS221 | U54 | 74LS04 |
| U11 | 74LS374 | U55 | 74LS393 |
| U12 | 81LS95/97 | | |
| U13 | 74LS175 | R1-R2 | 10K OHM |
| U14 | 81LS95/97 | R3 | 330 OHM |
| U15 | 74LS138 | R4 | 560 OHM |
| U16 | 74LS74 | R5-R7 | 5.1K OHM |
| U17 | 74LS04 | R8 | 560 OHM |
| U18 | 74LS174 | R9-R11 | 5.1K OHM |
| U19 | 74LS02 | R12 | 2.7K OHM |
| U20-U21 | 74LS74 | R13 | 4.7K OHM |
| U22 | 74LS266 | R14 | 470 OHM |
| U23 | 74LS10 | R15 | 5.1K OHM |
| U24 | 74LS00 | R16-R17 | 1.0K OHM |
| U25 | 74LS138 | SR1-SR6 | 4.7K OHM |
| U26-U28 | 1489 | | |
| U29-U30 | 4N28 | C1-C8 | DIPPED TANT 20V |
| U31-U33 | 1488 | C9-C10 | 220PF MICA |
| U34-U36 | 2651 | C11 | .01UF CERAMIC |
| U37 | 1488 | (25) | BYPASS CAPS |
| U38 | 1489 | X1 | 5.0688 MHz |
| U39 | 74LS04 | Q1 | 2N3904 |
| U40 | 74LS00 | D1 | SIGNAL DIODE |
| U41 | 74LS266 | LED1 | RED LED |
| U42 | 74LS04 | S1 | 10 POSITION |
| U43 | 74LS367 | S2 | 10 POSITION |
| U44 | 74LS138 | S3 | 8 POSITION |
| U45 | 81LS95/97 | JS1-JS3 | 8 POS. SHUNT |
| | | JS4-JS6 | 8 POS. HEADER |
| | | (10) | PIN SHUNTS |

The heading of the middle column is printed "INTERGRATED CIRCUITS" [sic].

### Component Layout (page 49)

A photograph of the component side of the board, "187A", with the legend: COPYRIGHT 1982
GODBOUT ELECTRONICS OAKLAND, CALIF. PARALLEL CONN 1, CENTRONICS CONN 2 and TRIPLE SERIAL CONN 3
(A, B, C) along the top edge; the regulators U1-U4 along the left edge; X1 and U55 at the top
right. It agrees with the drawing on page 6. The 2651s are U34, U35 and U36.

### Logic Diagram, page 1 of 8 (page 50): bus buffers and I/O decode

"INTERFACER 4 187C ©1982 COMPUPRO division GODBOUT ELECTRONICS". ◇ = SIP pull-up resistor.

- **Data in (U45, 81LS95/97):** DO0-DO7 (S-100 pins 36, 35, 88, 89, 38, 39, 40, 90) to the
  internal bus D0-D7, enabled by sOUT.
- **Data out (U46, 74LS244):** the internal bus to DI0-DI7 (S-100 pins 95, 94, 41, 42, 91, 92,
  93, 43). The low half is enabled by DOENL\* and the high half by DOENH\* (pins 1 and 19).
- **Address compare:** U41 and U22 (74LS266, open-collector XNOR) compare A7 (pin 83), A6 (82),
  A5 (29), A4 (30) and A3 (31) with S2-5..S2-9 (pulled up by SR5; a closed switch grounds the
  input). U22 compares sOUT with sINP\*. The outputs are wire-ANDed into ASEL. S2-4 grounds
  ASEL.
- **USEL:** U22 (XNOR) compares U18 Q (D3) with S2-1 and U18 Q (D4) with S2-2. The two outputs
  are wire-ANDed (R15 pull-up) into USEL. **H/L\* is not part of USEL.**
- **SEL\*:** U23 (3-input NAND) of ASEL, USEL, and NOT(A2·A1·ASEL). SEL comes from U17.
- **OUT7\*:** U23 (3-input NAND) of NOT(A2·A1·ASEL) inverted by U17, sOUT and STROBE. **A0 is not
  an input**, so relative ports 6 and 7 both write the user select latch.
- **U18 (74LS174):** D0-D5 clocked by OUT7\*, cleared by CLR\*. Q of D0 and D1 go to U25 A and B;
  Q of D2 is H/L; Q of D3 and D4 go to the USEL compare. D5 is wired to a D input; its output is
  not used on this page.
- **U25 (74LS138):** G1 = SEL, G2 = A2 and ESTROBE\*, A/B = the two user bits, C = SH/L\*. Outputs
  CE0\*, CE5\*, CE2\* and CE7\* (pins 15, 14, 13, 12, for inputs 0, 1, 2, 3 with C low).

**[sic]** The page 9 text says the five-bit register is "hex latch U18"; the parts list and the
drawing agree that U18 is a 74LS174, which is a hex latch.

The page 29 text says the user-select port write needs "ASEL, A1, A2, sOUT, and STROBE"; the
drawing agrees and shows no A0 term. The Port Map on page 8 calls relative port 6 "Not used".

### Logic Diagram, page 2 of 8 (page 51): strobes, data-out enables, reset, wait states

Read from the page image at 200 dpi.

- **U43 (74LS367)** buffers A2 (S-100 pin 81), A1 (80), A0 (79) and pDBIN (78).
- **DOEN\*** = U40 NAND(pDBIN, SEL).
- **STROBE\*** = U19 NOR(pDBIN, pWR\*) (pWR\* pin 77, inverted by U42); STROBE = U39 of STROBE\*.
- sOUT (pin 45) and sINP (pin 46) are inverted by U42 to sOUT\*/sOUT and sINP\*.
- **CLR:** U40 NAND(pRESET\* (pin 75), J9) gives CLR; U54 gives CLRVI\* and U39 gives CLR\*. J9
  connects SLVCLR\* (pin 54) or POC (pin 99) to the second input. With no J9 shunt (page 6), that
  input floats high and only pRESET\* clears the board. CLRVI\* clears the two mask latches.
- **ESTROBE\*:** U16 (D = VCC, clocked by STROBE\*), cleared by ESTATVAL\* = U40 NAND(pSTVAL\*
  inverted (pin 25), pSYNC (pin 76)).
- **U44 (74LS138):** A = A0, B = A1, C = sINP\*, G1 = A2, G2 = SEL\* and STROBE\*. Outputs IN4\*
  (Y0), IN5\* (Y1), OUT4\* (Y4), OUT5\* (Y5). Y2, Y3, Y6, Y7 are not used. Since SEL\* is false at
  A1·A2 (page 1), relative ports 6 and 7 are never read.
- **SH/L\*** = U5 XOR(H/L, S2-3). **HSEL** = the S2-3 level (SR1 pull-up; the switch ON grounds
  it). So HSEL = 1 with S2-3 OFF, the high four users of the group.
- **The full-byte enable:** U19 NOR(SH/L, A2), where SH/L = U39 of SH/L\*, then U19 NOR(that,
  DOEN\*). The result is true when DOEN\* is low AND (the user select matches this board's
  half OR A2 is high).
- **DOENL\*** = U24 NAND(that enable, U24 NAND(A2, HSEL)). **DOENH\*** = U24 NAND(that enable, U24
  NAND(A2, NOT HSEL)). So:
  - A2 = 0 (ports 0-3): both halves are enabled, but only on the board whose H/L matches.
  - A2 = 1 (ports 4-5): the low nibble on a board with HSEL = 0 (S2-3 ON), the high nibble on a
    board with HSEL = 1 (S2-3 OFF). Both boards of a group drive at once, one nibble each.
- **Wait states:** U20a, U21b, U21a form the shift register clocked by Φ\* (pin 24, inverted by
  U42), with STROBE on the D input of the first stage and on the clears. STALL1\*, STALL2\*,
  STALL3\* go to J6, J7, J8 (SR4 pull-up). U19 NOR(the selected STALL, A2), then U40 NAND(that,
  SEL), drives U43 open onto pRDY (pin 72). So waits only on ports 0-3.

**Conversion note:** USEL does not include H/L (page 1), so both boards of a group have SEL for
any of their eight users. The 2651 chip enables (U25, C = SH/L\*) and the full-byte data enable
(above) add the H/L match. The board of the other half also adds its wait states to the access,
which is why page 11 asks for the same wait count on every board.

### Logic Diagram, page 3 of 8 (page 52): interrupt status, masks, S3, VI lines, power

- **VI0-VI7** are S-100 pins 4-11. Pins 20 and 70 are ground.
- **Transmit status (U49, 74LS125, enabled by IN4\*):** DX0 = TxI4 (the Centronics interrupt
  flop), DX1 = TxRDY5 (U36), DX2 = TxRDY6 (U35), DX3 = TxRDY7 (U34).
- **Receive status (U53, enabled by IN5\*):** DX0 = RxI4 (the Universal Parallel input flop),
  DX1 = RxRDY5, DX2 = RxRDY6, DX3 = RxRDY7.
- **S3** joins DX0-DX3 to D0-D3 (positions 8, 7, 6, 5) or to D4-D7 (positions 4, 3, 2, 1).
- **U48 (74LS175, clocked by OUT4\*):** DX0-DX3 to TxINTEN4-7. **U52 (clocked by OUT5\*):**
  RxINTEN4-7. Both are cleared by CLRVI\*.
- **U51:** RxINT4\* = NAND(RxI4, RxINTEN4). **U47:** TxINT4\* = NAND(TxI4, TxINTEN4). (The serial
  channels' gates are on pages 6-8.)
- **JS5:** pins 9-12 and 13-16 are TxINT4\*-TxINT7\*; pins 8 down to 1 are VI0-VI7. **JS6:** the
  same for RxINT4\*-RxINT7\*. The page 13 table names these interrupts by relative user (0-3);
  the logic diagram names them 4-7.
- **U55 (74LS393):** CLOCK\* (pin 49) divided to 500K, 250K, 125K (first counter), 62.5K and
  31.25K (second counter, clocked from 125K).
- **Power:** U3 and U4 (7805) from pin 1/51 (+8 V) give +5 V; U2 (7812) from pin 2 (+16 V) gives
  +12 V; U1 (7912) from pin 52 (-16 V) gives -12 V. Pins 50 and 100 are ground.

**Conversion note:** the status and mask bits are wired to fixed chips: bit 0 is always the
parallel channels, bit 2 is always U35 (the middle channel). J26 (page 4 of the logic) swaps only
the chip enables of relative users 0 and 2. With J26 at A-C/B-D, user 2's registers are the
Centronics channel, but its status and mask bit is still bit 0, and bit 2 is the middle serial
channel that now answers as user 0. That agrees with the page 6 note: an interrupt-driven system
such as MP/M 8-16 must have J26 at A-B/C-D.

### Logic Diagram, page 4 of 8 (page 53): Universal Parallel, J26, baud clock

- **CONN1 (26 pins):** ST (6), DI0-DI7 (2, 15, 3, 16, 4, 17, 5, 18, with SR2 pull-ups), AT (8),
  DO0-DO7 (9, 22, 10, 23, 11, 24, 12, 25), EN (21, SR1 pull-up), +5V (7), +12V (13), -12V (1),
  ground (14, 19, 20, 26).
- **Input:** U6 (74LS373/374) latches DI0-DI7, clocked by U5 XOR(ST, S1-2). The same edge clocks
  U8a (D = VCC): its Q is **RxI4** and drives status D0 through U9 (enabled by CRD\*). U8a is
  cleared by U5 XOR(CLR, MRD\*): a read of the data, or reset. U6 drives D0-D7 when MRD\* is low.
- **Output:** U7 latches D0-D7 on MWR\* (inverted by U39). Its outputs are enabled by U5 XOR(S1-1,
  EN). U8b is set by MWR\* and cleared by the same enable; its Q goes to status D1 through U9 and,
  with Q\*, to J1. J1 joins AT (pin 8) to Q, Q\*, P or P\* (the MWR\* strobe itself or inverted).
- **J26:** A-B joins CE0\* to CPE\* (the parallel channels); C-D joins CE2\* to CE6\* (U35). A-C and
  B-D give CE0\* to CE6\* and CE2\* to CPE\*.
- **BAUDCLK:** X1 (5.0688 MHz) with U54 inverters, R16, R17, C11.

**[sic]** Page 22 names status D1 "TKN0, DATA TAKEN"; the drawing and page 32 show the flop that
a write sets and the enable clears, which is "data NOT taken" (DNTKN): D1 = 1 while the device
has not taken the byte.

### Logic Diagram, page 5 of 8 (page 54): the Centronics channel

- **U15 (74LS138):** A = A0, B = A1, C = sINP\*, G2 = CPE\* and STROBE\*. Read outputs: DRD\* (rel.
  0), SRD\* (1), MRD\* (2), CRD\* (3). Write outputs: DWR\* (0), SWR\* (1), MWR\* (2), CWR\* (3).
  CWR\* goes nowhere on these pages, so a write to relative port 3 of the parallel user does
  nothing.
- **Data:** U11 (74LS374) latches D0-D7 on DWR\* (inverted by U17) to CONN2 D1-D8 (pins 2-9;
  returns 20-27 to ground).
- **STROBE\*:** DWR\* fires one-shot U10a (R1/C9), which fires U10b (R2/C10, cleared by CLR\*);
  U9 buffers it to CONN2 pin 1. Page 32 gives 1 µs delay, then a 1 µs strobe.
- **Control:** U13 (74LS175, clocked by SWR\*, cleared by CLR\*) latches D0-D3. J2, J4, J3 pick Q
  or Q\* of D0, D1, D2 for AUTO FEED XT\* (pin 14), INIT\* (31) and SLCT IN\* (36). Q\* of D3 drives
  LED1 through R3 to VCC: D3 = 1 lights it, and reset turns it off.
- **Sense switch:** U14 (enabled by DRD\*) reads S1 positions 10, 9, 8, 7, 6, 5, 4, 3 onto D0-D7
  (SR4 pull-ups; a closed switch reads 0).
- **Status (U12, enabled by SRD\*):** D0 = BUSY (pin 11) inverted by U17. D1 = ground. D2 =
  ACKNLG\* (pin 10). D3 = PE (pin 12). D4 and D5 = ERROR\* (pin 32) inverted by U17. D6 = SLCT
  (pin 13). D7 = the inverted BUSY, the same signal as D0.
- **TxI4:** U16b, D = VCC, clocked from J5 (TOP: ACKNLG\*; BOTTOM: the inverted BUSY), cleared by
  DWR\*. Its preset is tied to VCC and its clear is DWR\* only: **CLR does not touch it**.

**Conversion notes:**

- PBMT (D0) and BUSY\* (D7) are one signal, "not busy". The printer's ACKNLG\* idles high, so
  D2 reads 1 between pulses. A ready printer gives Status-C = 0C5H, the value the Centronics test
  program waits for (page 38).
- TxI4 sets at the rising edge of ACKNLG\*, the end of the pulse (page 32: "at the end of an
  ACKNLG\* pulse"). Its state after power-up is not defined by the drawing.

### Logic Diagram, pages 6-8 of 8 (pages 55-57): the three USARTs

| Page | USART | Chip enable | Connector | Mode socket | Interrupts | Sync clocks |
|---|---|---|---|---|---|---|
| 6 | U34 | CE7\* | CONN3 A | JS1 | TxRDY7/RxRDY7, TxINT7\*/RxINT7\* (U47/U51 pins 11-13), J10 | J17-J20, J22, J24 |
| 7 | U35 | CE6\* | CONN3 B | JS2 | TxRDY6/RxRDY6, TxINT6\*/RxINT6\* (U47/U51 pins 1-3), J11 | J13-J16, J21, J23 |
| 8 | U36 | CE5\* | CONN3 C | JS3, JS4 | TxRDY5/RxRDY5, TxINT5\*/RxINT5\* (U47/U51 pins 4-6), J12 | none (RxC and TxC unconnected) |

Common to all three:

- D0-D7, A0, A1, sINP\* to R/W\*, CLR to RESET, BAUDCLK to BRCLK.
- TxD, RTS\* and DTR\* drive 1488 sections; RxD, CTS\*, DSR\* and DCD\* come through 1489 sections.
  CTS, DSR and DCD have +12 V pull-ups (SR3, or R9-R11 on page 7), so an open input is ON and
  the 2651 sees it asserted.
- The socket (JS1-JS3) pins 1-8 carry TxD, RxD, RTS, CTS, DSR, DTR, DCD and a +12 V pull-up; pins
  16-9 go to DB-25 pins 3, 2, 5, 4, 20, 6, 8 and (pin 9) -12 V through a resistor. A straight
  shunt is the slave (DCE) wiring.
- TxRDY\* and RxRDY\* (SR6 pull-ups) are inverted by U50 to TxRDYn and RxRDYn, which go to the
  status buffers and to the U47/U51 NAND gates with TxINTENn and RxINTENn.
- J10, J11, J12 join TxEMT/DSCHG\* to TxRDY\* (T) or RxRDY\* (R).

Page 8 adds the current loop on U36: JS4, J25, opto-isolators U29 (input) and U30 (output), Q1,
D1, R4, R8, R12-R14; DB-25 +IN (14), -IN (15), +OUT (16), -OUT (17), TTL IN (18), TTL OUT (19).
The pin 18 of U36 is labelled "TxEMT/DSCH6" [sic].

### Jumper and Switch Index (page 58)

| Jumper | Section | Function | Page |
|---|---|---|---|
| J1 | UNIVERSAL PARALLEL | ATTENTION LINE | 23-24 |
| J2-J5 | CENTRONICS PARALLEL | CONTROL-C LINES | 27-28 |
| J6-J8 | HARDWARE | WAIT STATE SELECT | 11 |
| J9 | NOT USED | | |
| J10-J12 | SERIAL | INTERRUPTS | 14 |
| J13-J16 | SERIAL | SYNC CLOCKS REL 2 | 20-21 |
| J17-J20 | SERIAL | SYNC CLOCKS REL 3 | 20-21 |
| J21-J22 | SERIAL | SYNC CLOCKS | 20-21 |
| J23-J24 | SERIAL | SYNC BAUD SELECT | 21 |
| J25 | SERIAL | CURRENT LOOP | 19 |
| J26 | HARDWARE | SWAP OPTION | 11 |
| JS1 | SERIAL | MODE: REL. 3 | 19 |
| JS2 | SERIAL | MODE: REL. 2 | 19 |
| JS3 | SERIAL | MODE: REL. 1 | 19 |
| JS4 | SERIAL | MODE: REL. 1 | 19 |
| JS5 | HARDWARE | Tx INTERRUPTS | 13-14 |
| JS6 | HARDWARE | Rx INTERRUPTS | 13-14 |
| S1/1-2 | UNIVERSAL PARALLEL | STROBE POLARITY | 22-23 |
| S1/3-10 | CENTRONICS PARALLEL | SENSE SWITCH | 28 |
| S2/1-10 | HARDWARE | ADDRESSING | 8-9 |
| S3/1-8 | HARDWARE | BUS SELECT | 10 |

**[sic]** J9 is "NOT USED" here, but the logic diagram (page 51) shows J9 choosing SLVCLR\* or POC
as a second clear. S1-1 is listed as "STROBE POLARITY"; page 23 and the drawing make S1-1 the
OUTPUT ENABLE polarity and S1-2 the strobe polarity.

## Back cover (PDF page 58)

IF YOU NEED ASSISTANCE ALWAYS CONTACT YOUR COMPUPRO DEALER FIRST

CUSTOMER SERVICE INFORMATION: Our paramount concern is that you be satisfied with any Godbout
CompuPro product. If this product fails to operate properly, it may be returned to us for
service; see warranty information below. If you need further information feel free to write us
at: Box 2355, Oakland Airport, CA 94614-0355

LIMITED WARRANTY INFORMATION: Godbout Electronics will repair or replace, at our option, any
parts found to be defective in either materials or workmanship for a period of 1 year from date
of invoice. Defective parts MUST be returned for replacement.

If a defective part causes a Godbout Electronics product to operate improperly during the 1 year
warranty period, we will service it free (original owner only) if delivered and shipped at
owner's expense to and from Godbout Electronics. If improper operation is due to an error or
errors on the part of the purchaser, there may be a repair charge. Purchaser will be notified if
this charge exceeds $50.00.

We are not responsible for damage caused by the use of solder intended for purposes other than
electronic equipment construction, failure to follow printed instructions, misuse or abuse,
unauthorized modifications, use of our products in applications other than those intended by
Godbout Electronics, theft, fire, or accidents.

Return to purchaser of a fully functioning unit meeting all advertised specifications in effect
as of date of purchase is considered to be complete fulfillment of all warranty obligations
assumed by Godbout Electronics. This warranty covers only products marketed by Godbout
Electronics and does not cover other equipment used in conjunction with said products. We are
not responsible for incidental or consequential damages.

Prices and specifications are subject to change without notice, owing to the volatile nature and
pricing structure of the electronics industry.

"INTERFACER 4" is a trademark of W.J. Godbout.

"INS2651 PROGRAMMABLE COMMUNICATIONS INTERFACE", copyright 1980, National Semiconductor
Corporation. Reprinted reprinted [sic] by permission of National Semiconductor Corporation.

Copyright ©1982 by Godbout Electronics. All rights reserved. We encourage quotation for the
purposes of product review if source is credited. Printed in U.S.A.

COMPUPRO A GODBOUT COMPANY • 3506 BREAKWATER CT., HAYWARD CA 94545

---

## What this means for the pascal100 model

Facts from the pages above, in the order the model needs them. Page numbers are printed pages.

1. **The block and the latch are the Interfacer 3's.** Eight ports, base on S2-5..9 (A7-A3,
   ON = 0), S2-4 ON disables. The user select latch takes D0-D4 on a write to relative port 6 or
   7 (A0 not decoded, page 50), and every board at the base latches it. An IN at 6/7 is not
   decoded. Reset clears the latch (U18) and the masks (U48/U52).
2. **A board holds four users, half a group.** S2-1/S2-2 pick the group of eight (D3-D4), S2-3 the
   half (D2): OFF = the high four (relative users 4-7 of the group, "HSEL"). Two Interfacer 4s,
   one per half, make one group. An Interfacer 4 and an Interfacer 3 can share a base but not a
   group (page 7: intermix "at the same port addresses").
3. **Ports 0-3** reach the selected user only on the board whose group AND half match. Ports 4
   and 5 answer on both boards of the matching group (USEL has no H/L term); each drives its own
   nibble (S3 and HSEL, pages 10 and 51). With no board in the other half, that nibble reads
   1s (page 15: 0F0H or 0FH with no interrupt pending).
4. **Writes to ports 4/5** load each board's mask from its own nibble (S3). One write of 52H
   sets the masks of both halves (page 14).
5. **S3 must agree with S2-3** (page 10). The model can take the nibble from S2-3 and leave S3
   out, or keep S3 as a separate property and refuse a mismatch. The page 10 warning ("possible
   board damage") is a reason to refuse it.
6. **Relative user 0** (standard J26) is the two parallel channels:
   - Port 0: write = Centronics data (strobed out 1 µs later, for 1 µs); read = sense switch S1-3..10
     (D7..D0, ON = 0).
   - Port 1: read = Status-C (D0 not busy, D1 0, D2 ACKNLG\*, D3 PE, D4/D5 error, D6 SLCT, D7 not
     busy); write = Control-C (D0 AFX, D1 INIT, D2 SLCTIN, D3 LED), cleared by reset.
   - Port 2: write = Universal Parallel output latch (sets DNTKN); read = its input latch (clears
     DAV/RxI4).
   - Port 3: read = Status-P (D0 DAV, D1 DNTKN); a write does nothing.
7. **Relative users 1-3** are 2651s: 1 = U36 (CONN3 C, current loop option), 2 = U35 (CONN3 B),
   3 = U34 (CONN3 A). The same Sig2651 model as the Interfacer 3 and SS1.
8. **Status and interrupt bits** per board (nibble bit n): TxRDY/TxI and RxRDY/RxI of bit 0 =
   parallel (TxI4 = the Centronics acknowledge flop, RxI4 = the Universal Parallel input flop),
   bits 1-3 = U36, U35, U34. The interrupt is the RDY AND the mask bit, to any VI line through
   JS5/JS6.
9. **J26 at A-C/B-D** swaps the chip enables of relative users 0 and 2 only; the status and mask
   bits stay on the chips (page 52). The model can offer the swap with that exact effect.
10. **TxI4** sets at the end of each printer ACKNLG\* pulse (J5 TOP) or when BUSY falls (J5
    BOTTOM), and clears on a data write, not on reset. A model needs a power-up value for it;
    the drawing gives none.
11. **Wait states:** 0-3 (J6/J7/J8) on ports 0-3 only.
12. **Not to model:** synchronous mode, the external sync clocks and U55, the current loop, the
    TxEMT/DSCHG jumpers J10-J12, the Universal Parallel strobe and attention polarities
    (S1-1, S1-2, J1), J9, and the LED other than as a visible state.
