# CompuPro DISK 2 & SELECTOR CHANNEL Technical Manual

Source: `CompuPro Disk 2 Manual.pdf` (a scan with no text layer). "DISK 2 & SELECTOR
CHANNEL Technical Manual", CompuPro, a Godbout Company, Hayward CA. Copyright 1982. First edition
February 1982, second edition April 1983, **third edition November 1983**. File name DISK2.MAN,
**Document # 11721**, **Board No. 172/177, Revision D** (172 = Selector Channel, 177 = DISK 2).
Cover: "ARBITRATED 24 BIT DMA HARD DISK CONTROLLER for Fujitsu 2300 Series Drives", IEEE 696/S-100,
$20.00, code A192.

47 PDF pages, a scan with **no text layer**. The text was taken from the OCR layer of a second scan
of the same edition, `Disk2_SelectorChannel_Manual_Nov83_bitsavers_OCR.pdf` (bitsavers)
(49 pages; it has two more blank pages, after the contents and after the "DISK 2" title page), and
**every page was checked against the first scan**. Where they differ, the scan wins. Converted
2026-11-14.

Page numbers: PDF pages 1-4 are the cover, the copyright page, the contents and the "DISK 2" title
page. Printed pages 1-41 follow, so **the PDF page is the printed page + 4**. PDF page 46 is the
customer service and warranty page, 47 the back cover. Printed page 30 (PDF 34) is blank.

Conventions are the same as the Disk 1 conversion ([CompuPro Disk 1 & 1A Floppy Controllers.md](CompuPro%20Disk%201%20%26%201A%20Floppy%20Controllers.md)): the image wins over
the OCR, hex is written with zeros, errors in the printed manual are kept and marked **[sic]**, and
notes that interpret the manual say so. An asterisk after a signal name (`ATTN*`) means active low,
as in the manual.

**Related CompuPro documents.** The board settings for MP/M are in CompuPro's *MP/M 8-16 Technical
Manual* (March 1984); the DISK2 utility under CP/M-86 in the *CP/M-86 Technical Manual*. The driver
source that programs this board is CompuPro's CP/M 8-16 V1.1R `TMXDISK2.DVR/.TBL/.INI` with
`COMPUPRO.EQU`; it is not part of this manual.

## Contents (PDF page 3)

| Section | Printed page |
|---|---|
| **DISK 2 section** | |
| About Disk 2 | 1 |
| Technical Overview | 1 |
| Switch Setting and Option Selection | 2 |
| Formatting a Hard Disk | 3 |
| Jumper & Switch Settings | 4 |
| Programming the Disk 2 | 5 |
| Control/Status Register | 5 |
| Theory of Operation | 11 |
| On the Bus Side: I/O Ports | 11 |
| On the Bus Side: DMA Interface | 11 |
| On the Disk Side: Disk Interface | 13 |
| In the Middle: Disk data | 14 |
| Hardware Section | 19 |
| Logic Diagram | 20-27 |
| Parts List | 28 |
| Component Layout | 29 |
| **Selector Channel section** | |
| About Selector Channel | 31 |
| Technical Overview | 31 |
| Switch Settings and Option Selection | 31 |
| Programming the Selector Channel | 32 |
| Theory of Operation | 33 |
| Hardware Section | 35 |
| Logic Diagram | 36-39 |
| Parts List | 40 |
| Component Layout | 41 |
| **Service and Warranty Information** | |

# DISK 2 section

## About Disk 2 (page 1)

Congratulations on your purchase of the DISK 2 hard disk controller and the SELECTOR channel. These
boards are designed for maximum performance applications featuring full physical and electrical
compatibility with the IEEE 696/S-100 specifications. The boards implement a temporary master
interface capable of transferring data directly between a hard disk and memory at data rates up to
1 Mbyte per second. The boards may directly access the full 16 Mbyte extended address range of the
bus while requiring only three I/O port addresses for their operation.

## Technical overview (page 1)

The DISK 2™ board features a standard IEEE 696/S-100 edge connector along its bottom edge and a set
of ribbon cable connectors along the top which will directly accept up to four Winchester type hard
disk drives selected from a variety of manufacturers, models and capacities. The DISK 2 contains
all of the hardware necessary for control of the drives including the ability to format. The board
also contains the necessary hardware to arbitrate for and accept the bus as a temporary master per
the IEEE 696/S-100 specification and to transfer data thereon. The SELECTOR channel contains all of
the hardware for generation of address, status and control signals required for temporary master
operation of the DISK 2. The two board combination functions exactly as a temporary master per the
bus specifications. No interconnections (other than the bus itself) are required between the boards
for operation and all bus signals are used in their normal way. The DISK 2 performs no useful
function without a SELECTOR channel, but the SELECTOR channel may be shared by other controllers
designed to use it.

## Switch settings and option selection (page 2)

ON = "0", OFF = "1" on both switches.

**Switch 1**

| Position | Label | Function |
|---|---|---|
| 1 | - | Not used |
| 2 | P3 | DMA priority (MSB) |
| 3 | P2 | DMA priority |
| 4 | P1 | DMA priority |
| 5 | P0 | DMA priority (LSB) |
| 6 | 20 | 2048 byte sectors |
| 7 | 10 | 1024 byte sectors |
| 8 | 5 | 512 byte sectors |
| 9 | 2 | 256 byte sectors |
| 10 | 1 | 128 byte sectors |

Positions 6-10: only one on at any time.

**Switch 2**

| Position | Label | Function |
|---|---|---|
| 1 | A7 | Port address (MSB) |
| 2 | A6 | Port address |
| 3 | A5 | Port address |
| 4 | A4 | Port address |
| 5 | A3 | Port address |
| 6 | A2 | Port address |
| 7 | A1 | Port address (LSB) |
| 8 | - | Not used |

**PORT SELECTION:** The DISK 2 is controlled through two I/O ports. A pair of consecutively
addressed ports are selected by setting SW2-1 through SW2-7 according to the seven most significant
bits of the desired port address. The CompuPro standard for the DISK 2 is ports C8 and C9.

**DMA PRIORITY:** When requesting the bus for transfer of data as a temporary master, the DISK 2 will
arbitrate with a priority which is set by switches SW1-2 through SW1-5. Priority 15 is the highest
while 0 is the lowest. When assigning a priority to the DISK 2 bus transfers, all other DMA devices
must be taken into consideration. Then, if the DISK 2 has a higher priority, when any conflict
occurs the DISK 2 will win the arbitration. If the other requesting device can complete its
operation in only a short time, it could be overall faster to give the slower device a higher
priority. Latency time for each DMA device and the amount of bus access time required by each DMA
device will help determine which device should have which priority for optimum system throughput.

**SECTOR SIZE:** The DISK 2 may read, write and format sector sizes from 128 bytes to 2048 bytes. The
desired sector size is selected by SW1 positions 6-10 according to the table. Only one of these
switches may be on at a time. (Changing sector sizes also requires adjustments in the drives and
software, and reformatting the drives).

In addition to setting switches as described above, a number of options are selected by means of
jumpers.

> Conversion note: the manual says "three I/O port addresses" on page 1 and "two I/O ports" here.
> Both are right: the DISK 2 uses two ports (C8, C9) and the Selector Channel one more (its own
> port; see page 31).

## Interrupts, write clock, formatting (page 3)

**INTERRUPT VECTOR SELECTION:** Use of interrupts is not required by the CP/M supplied with the
DISK 2/SELECTOR CHANNEL board set. But, if the use of an interrupt is desired to allow the CPU to
process other tasks concurrent with a DISK 2 operation, installation of a jumper in one of the
locations from J7 to J14 will result in ATTN, (the "done" flag) asserting one of the vectored
interrupt lines VI0 through VI7 respectively. The flag may be reset through the control register
(See Programming the Disk 2). If the use of an interrupt is desired, and a vectored interrupt
controller is not available, jumper J15 will result in ATTN asserting INT. An interrupt on this line
may be sensed directly by the CPU. Note that ATTN will come up asserted after a RESET.

| Jumper | ATTN drives |
|---|---|
| J7 ... J14 | VI0 ... VI7 (J7 = VI0, J14 = VI7) |
| J15 | INT |

**WRITE CLOCK PHASE:** All of the Shugart compatible drives (such as the Memorex 100 series and the
Fujitsu 2300 series) require jumper J6 (between U25 & U26) in the C\* position while Shugart SA4000
series drives require that J6 be in the C position.

### Formatting a hard disk

Before using your hard disk, you must format it with the "DISK2" utility provided with your CP/M 80
and CP/M 86 system disks. The "DISK2" program will format, verify and test your hard disk drive. The
different options that can be used are in the following list.

USAGE: `DISK2 {options}`

At least one option must be specified such as the drive type. You can mix any of the options.

| Option | Meaning |
|---|---|
| `m10` | Set drive type to Fujitsu 10 Mbyte |
| `m20` | Set drive type to Fujitsu 20 Mbyte |
| `m26` | Set drive type to Shugart 26 Mbyte \* |
| `m10m` | Set drive type to Memorex 10 Mbyte \* |
| `drive #` | Format selected drive |
| `format` | Format headers |
| `data` | Write out data fields with E5H |
| `test` | Perform a data field test |
| `seek` | Perform a seek test |
| `all` | Perform format, data test, seek test |
| `skew #` | Set skewing of disk to specified number. This option goes along with hardware settings. |

\* **Although the DISK 2 program will format these drives, The CompuPro CP/M 80 and CP/M 86 systems
are not configured for them.**

## Defaults, examples, drive jumpers (page 4)

If an option is not specified then the following defaults are used:

- Defaults: M20, skew of 2, sector size 1024, drive a, no tests or formatting.
- Examples:
  ```
  disk2 format data
  disk2 m20 all
  disk2 data
  disk2 format data drive 1 skew 3 m10
  ```

### Jumper & switch settings (on the drive)

Check to see if your Memorex 100 has the Fujitsu 2300 LSI or MSI board revision.

These are the switches and jumpers **on the drive's own logic board**, not on the DISK 2. The
drawings are converted to tables (one row per switch position).

**LSI Fujitsu 2300 & new Memorex 100 series.** Jumper S1: 2 to 3.

| Position | SW1 | SW2 | SW3 | SW4 | SW5 |
|---|---|---|---|---|---|
| 1 | OFF | OFF | ON | OFF | ON |
| 2 | ON | OFF | ON | OFF | ON |
| 3 | OFF | ON | OFF | OFF | ON |
| 4 | OFF | ON | OFF | OFF | OFF |
| 5 | OFF | OFF | OFF | OFF | |
| 6 | OFF | ON | OFF | OFF | |
| 7 | ON | OFF | OFF | OFF | |
| 8 | OFF | OFF | ON | OFF | |

**MSI Fujitsu 2300.** Jumpers S1: 2 to 3; S2: 1 to 2; S3: 1 to 2.

| Position | SW1 | SW2 | SW3 | SW4 |
|---|---|---|---|---|
| 1 | OFF | OFF | ON | OFF |
| 2 | ON | OFF | ON | OFF |
| 3 | OFF | ON | OFF | OFF |
| 4 | OFF | ON | OFF | OFF |
| 5 | OFF | OFF | OFF | OFF |
| 6 | OFF | ON | OFF | OFF |
| 7 | ON | OFF | OFF | OFF |
| 8 | OFF | OFF | ON | OFF |

SW1-SW4 are the same on both revisions; the LSI board adds SW5.

## Shugart SA4000 jumpers (page 5)

Jumpers on the Shugart SA4000 drive:

| Position | 1 (LSB) | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 (MSB) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Setting | out | out | in | in | in | out | out | in | in | in | out | in |

| Jumper | Setting |
|---|---|
| X | out |
| DS1/2,3,4 | in/out |
| ST | in |
| RY | in |
| IX | in |
| T | in |
| C | out |
| SC / BC | in / out |
| D | in |
| E | out |
| S1/S2 | in/out |
| 4H | shunt out |
| E1, E2, E3 | no conn. |
| R | in |
| S | out |
| F | out |
| SL | in |
| GR | in |
| DL1 | out |

## Programming the DISK 2 (pages 5-10)

All functions of the DISK 2 are controlled through a pair of I/O ports whose addresses are selected
by SWITCH 2. The lower addressed port is the control/status port. Data written to this port is
placed in the CTL register while reading this port will return the contents of the STATUS register.
The upper port is the DATA port. Writing to this port will send a byte of data to a data register
previously selected by setting the CTL register. Reading the DATA port will cause a STEP pulse to be
sent to the previously selected drive (drive selection will be discussed below). No valid data is
returned by the read.

| Port | Write | Read |
|---|---|---|
| base (C8H) | CTL register | STATUS register |
| base + 1 (C9H) | the data register picked by OP1/OP0 | sends one STEP pulse; no valid data |

### Control/status register

| Bit | CTL (write) | STATUS (read) |
|---|---|---|
| 7 | ATTN\* | ATTN\* |
| 6 | RUN | TIME OUT |
| 5 | OP2 | CRC ERROR |
| 4 | OP1 | OVER RUN |
| 3 | OP0 | READY\* |
| 2 | FAULT CLR | SEEK COMPLETE\* |
| 1 | US1 | WRITE FAULT\* |
| 0 | US0 | TRACK 0\* |

\* indicates an active low signal.

While the STATUS register may be read at any time, it might be in either a transparent or a latched
state. If it is transparent, the data returned is the current state of the register's inputs. If it
is latched, data returned was the input data at the time the register was latched. The current state
is indicated by the ATTN\* bit, ATTN\* true (low) indicating a latched condition. The ATTN\* bit may
be controlled through the CTL register to make the STATUS register transparent.

### Data registers (page 6)

There are four registers which may be written through the DATA port. To write a specific register,
the register number is first stored in the CTL register bits OP1 and OP0 as shown in the table
below. This will direct a byte of data written on a subsequent transfer via the DATA port to the
appropriate register.

| OP1 | OP0 | Data register |
|---|---|---|
| 0 | 0 | DRIVE |
| 0 | 1 | CYLINDER |
| 1 | 0 | HEAD |
| 1 | 1 | SECTOR |

The table adds "See status/control chart above for bit assignment".

**DRIVE SELECTION AND STATUS:** Up to four drives may share the control cable which is connected to
the 50 pin ribbon connector and runs "daisy chained" to each of the drives. There is a unique DRIVE
SELECT line in the cable for each of the drives. Only one of these select lines may be active at a
time. The selected drive will respond to the control output lines on the cable and drive its status
onto the input lines. The select lines are driven from the four high order bits of the DRIVE
register. Bit 4 selects the first drive, bit 5 the second etc. The READY\* bit in the status
register is driven by the READY line of the selected drive. Similarly the status lines WRITE
FAULT\*, SEEK COMPLETE\* and TRACK 0\* all reflect the status of the selected drive.

**HEAD POSITIONING:** Most drives require that the controller software remember the current track.
To position the read/write heads at a new track, the drive is selected and the DATA port is read a
number of times equal to the difference between the current track and the new track. Each read from
the DATA port will send a STEP pulse to the selected drive causing its heads to move one track. The
direction of the STEP is controlled by the OP2 bit of the CTL register. OP2 in the CTL register
drives the DIRECTION\* line in the control cable. Most drives use stepping motors or other
positioning devices with limited step rates. (See drive specifications for maximum step rates).

The starred (\*) paragraphs below are features that may be available on your drives and are for use
by the more experienced system programmer.

> \* Many of the newer drives feature a fast seek mode. These drives will accept and accumulate STEP
> pulses at a very high rate without moving the access mechanism. After all of the pulses are
> received, the SEEK is performed at a much higher average track per second rate than would be
> achieved stepping one track at a time. (See drive specifications.) The DISK 2 board can
> accommodate this feature if your drive has it.

**HEAD SETTLING TIME (page 7):** After a seek, the hard disk requires a significant amount of time
(in the order of one revolution) for the heads to settle before reading or writing is attempted. The
software must provide this delay starting with the receipt of SEEK COMPLETE. This time period can be
found in your drive specifications.

> \* Some of the newer drives provide a jumper option which will delay the SEEK COMPLETE indication
> by the required head settling period eliminating this burden from the drive software.

> \* The DISK 2 automatic functions are interlocked to SEEK COMPLETE. As an alternative to the use of
> a timer for generation of the head setting **[sic]** delay in a multiprogramming application, the
> disk software may issue a NULL command immediately following the step pulses of the seek. The NULL
> command will not begin until the drive indicates a SEEK COMPLETE. NULL has no function other than
> to fail with a TIME OUT indication after two index pulses from the drive. This completion would
> interrupt the CPU indicating that the drive was ready to accept a read or write command. If the
> available drives include the option of delaying SEEK COMPLETE by the head settling time, the disk
> software may issue a READ DATA or WRITE DATA immediately following the step pulses. Again, the
> interlock will hold off the automatic operation until SEEK COMPLETE is indicated.

**READ SELECTION [sic]:** Prior to any data transfer operation to the disk, the software must select
a read/write head. The drives will have at least one and sometimes two heads for each surface of
each platter used for data storage. The control cable includes 4 head address lines. The lines are
driven by the lower four bits of the DRIVE register, bit 0 being the least significant. The required
nibble selecting one of up to sixteen heads, as appropriate for the drive, will normally be stored
in the DRIVE register at the same time that the drive is selected since this is the function of the
upper nibble in the register.

> Conversion note: the heading is printed "READ SELECTION"; the paragraph is about **head**
> selection.

So the DRIVE register is:

| Bits 7-4 | Bits 3-0 |
|---|---|
| drive select, one bit per drive (bit 4 = first drive, bit 5 = second, ...) | head address 0-15 to the cable |

The HEAD data register (OP1/OP0 = 10) is only compared with the header (see READ DATA below); the
head lines on the cable come from the DRIVE register.

**AUTOMATIC OPERATIONS:** On the disk, each data field is preceded by a unique header field which
was written when the disk was formatted. A read or write operation consists of searching the entire
track of the currently selected drive and head for the header record of the desired sector. Once the
header is found, the data field immediately following the header may be read or written. Because of
the high data transfer rate of the disk, all of these operations occur too quickly for direct
software control. For this reason, the DISK 2 contains an FSM (finite state machine) which performs
these functions automatically.

### The NULL command (command 00, page 8)

This command was mentioned above under head settling time however its primary application is to
align sectors relative to index during formatting. It is issued in the same way as a READ DATA
except with a NULL operation code. The CYLINDER, HEAD and SECTOR register are not required and no
SELECTOR CHANNEL programming is required because no data is transferred. A ready drive must be
selected as a source for index or the DISK 2 will fail to complete the operation.

### The READ DATA command (command 01)

Prior to issuing a READ DATA command, the location of the required sector must be determined and the
drive, track and head selection processes completed so that once the read begins, the sector may be
found. These selection processes have all been described above. Since the READ DATA will be
performed automatically, the memory address to which the data will be transferred and the identity
of the sector header must all be determined before the operation is begun. (See the SELECTOR CHANNEL
manual for programming considerations). The header fields contain a three byte code. During a
search, these bytes are compared against the contents of the CYLINDER, HEAD and SECTOR registers.
These are DATA registers whose contents may be loaded as described above. The contents of the
CYLINDER, HEAD and SECTOR registers have no affect **[sic]** on the selection process, they are only
used as a source of data for matching headers.

Once all of the data has been loaded, the command byte may be written to the CTL register. The
command byte must contain the following:

| Field | Value |
|---|---|
| US1 & US0 | These bits must contain the drive number and be consistent with the contents of the DRIVE register. |
| FAULT CLR | Must be 0. |
| OP2 - OP0 | Must contain the operation code for the desired function. (See Op Code Table). |
| RUN | Must be 1 indicating an automatic operation. |
| ATTN\* | Must be a 1 (false) making the status register transparent and indicating busy. |

Once the command has been issued, the CPU is free to perform other tasks. The DISK 2 will transfer
the sector to memory automatically once it is found. Completion of the operation is indicated by the
ATTN\* bit of the STATUS register which may optionally result in an interrupt.

### Result status (page 9)

**RESULT STATUS:** It is possible that an automatic operation may not complete satisfactorily. After
any automatic operation, the STATUS register should be read, and it **[sic]** contents which were
latched at the moment of completion should be evaluated. The resulting status from a successful
read will be a 3 (or a 2 if track 0 is being read). If an error has occurred, the bits from the
STATUS register should be evaluated with the following priority:

1. **READY\*** (Bit 3) If the drive had lost its ready state, no data transfers may occur. TIME OUT
   will probably also be true. Check for wrong drive selected. May also be caused by a power line
   dip or drive failure.
2. **TIME OUT** (Bit 6) Automatic commands will be terminated and the time out indication set if two
   index pulses are received from the drive before an operation is completed. This allows between
   one and two complete passes of the track to find a header. TIME OUT would indicate that the
   header was not found. Since soft errors may occur, the original command should be retried a few
   times. If the TIME OUT persists, a READ HEADER command may be used to read any header on the
   track. This header found would indicate if the wrong track or head was being searched. A wrong
   track could result from a drive seek error. If the track was wrong, the drive should be homed to
   track 0 and the seek performed again. If all of the above are correct and the header is still
   unreadable, a hard error is indicated.
3. **OVER RUN** (Bit 4) This bit indicates that the request for use of the bus was not acknowledged
   quickly enough and some or all of the data may have failed to make it to memory. If the OVER RUN
   was due to collisions with other DMA devices or CPU response to the bus request, it may be
   resolved through retries. If this status persists and none of the data was transferred, the
   SELECTOR CHANNEL programming and bus DMA capability should be verified. If some of the data is
   being transferred, possibly incorrect, the combination of the bus clock rate and memory speed as
   controlled by wait states may not be fast enough for the drive being used.
4. **CRC ERROR** (Bit 5) This bit indicates that the CRC check word computed on the data as it was
   read from the disk did not match the check word which followed the data field. This again may
   result from a "soft" read error. The operation should be retried.

> Conversion note (interpretation): status 3 = 0000 0011B: ATTN\* = 0 (latched, done), no error
> bits, READY\* = 0 and SEEK COMPLETE\* = 0 (true, active low), WRITE FAULT\* = 1 and TRACK 0\* = 1
> (false). On track 0, TRACK 0\* = 0 gives 2. This agrees with the bit table on page 5.

### The WRITE DATA command (command 02, page 10)

The WRITE DATA procedure is the same as the READ DATA procedure except the command sent to the CTL
register must have the write operation code in the OP2 - OP0 bits. An additional status may occur on
a write operation, namely WRITE FAULT. This status signal is bit 2 **[sic]** in the STATUS register.
This status may result from attempting some kind of illegal operation but would normally indicate a
drive malfunction. WRITE FAULT must be reset through FAULT CLEAR in the CTL register before the disk
will respond to any new commands. After each write operation the status should be checked and if
there are any errors they should be checked in the same priority as during a read operation with
the WRITE FAULT error checked just after the READY\* bit.

> Conversion note: the table on page 5 puts WRITE FAULT\* at **bit 1**; bit 2 is SEEK COMPLETE\*.
> The page 9 rule (status 3 on a good read) agrees with the table, so "bit 2" here is a misprint.

### The WRITE HEADER command (command 03)

This is the primitive command used in formatting a drive. The command will write a sector header
record upon receipt of the next sector pulse from the disk. The three data bytes for the field will
be read directly from memory. To format a track, the program must issue a sequence of WRITE HEADER
commands, one for each sector on the track. Each command must be issued immediately after the
preceding one completes so that it will be pending when the next sector pulse is received from the
disk. As an aid to issuing the commands quickly, the data field for all of the headers on a track
should be computed in advance and placed in a buffer. As each command is executed, it advances the
address register in the SELECTOR CHANNEL by three bytes leaving it pointed at the data for the next
field. If tracks are to be formatted with sectors in a specific position relative to index, the
first WRITE HEADER command should be issued immediately upon completion of a NULL command (which
occurs at index). While the data placed in the header fields is arbitrary as far as the hardware is
concerned, the cylinder, head and sector numbers are recommended. (See track format and state
timing.)

### The READ HEADER command (command 04)

As described above in resolving a TIME OUT status, one may issue a READ HEADER command. This command
will transfer the three data bytes of the next header field which passes under the head directly to
memory. The requirements for issuing the command are the same as for a READ DATA except the
CYLINDER, HEAD and SECTOR registers need not be loaded and only three bytes will be transferred. As
in a READ DATA COMMAND, error checking should be done after each operation.

### Operation codes (summary of pages 8-10)

The "Op Code Table" that page 8 refers to is not printed in this edition. The command numbers in the
headings give it:

| Command | OP2 OP1 OP0 | Operation | Data moved by the Selector Channel |
|---|---|---|---|
| 00 | 0 0 0 | NULL (wait for SEEK COMPLETE, then end at index or TIME OUT) | none |
| 01 | 0 0 1 | READ DATA | one sector, disk to memory |
| 02 | 0 1 0 | WRITE DATA | one sector, memory to disk |
| 03 | 0 1 1 | WRITE HEADER | 3 header bytes, memory to disk |
| 04 | 1 0 0 | READ HEADER | 3 header bytes, disk to memory |

> Conversion note: the manual does not print this table; it is the command number written as a
> 3-bit OP2-OP0 field. **Checked against the CP/M 8-16 V1.1R source** (`COMPUPRO.EQU`, outside this
> manual): `D2_READ EQU 11001000b` (ATTN\* = 1, RUN = 1, OP = 001) and `D2_WRT EQU 11010000b`
> (OP = 010) agree. The same file confirms the rest of this section:
>
> | Equate | Value | Meaning |
> |---|---|---|
> | `D2_PORT` / `D2_CNTL` / `D2_STAT` | 0C8H | CTL (write) and STATUS (read) |
> | `D2_DATA` | 0C9H | DATA port |
> | `D2_STRB` | 10000000B | select the DRIVE register (OP1/OP0 = 00) |
> | `D2_CYL` / `D2_HEAD` / `D2_SEC` | 10001000B / 10010000B / 10011000B | select CYLINDER / HEAD / SECTOR (01 / 10 / 11) |
> | `D2_SIN` / `D2_SOU` | 10100000B / 10000000B | step in = OP2 = 1, step out = OP2 = 0 |
> | `D2_RST` | 00000100B | FAULT CLR |
> | `D2_WRTF` | 00000010B | WRITE FAULT is STATUS **bit 1** (so "bit 2" on page 10 is a misprint) |
> | `SELCHAN` | 0F0H | the Selector Channel port |
>
> With RUN = 0 the OP bits only pick the data register (OP1/OP0) and the step direction (OP2).

## Theory of operation (pages 11-18)

### On the bus side: the I/O ports (page 11)

The S-100 bus data strobes PWR\* and PDBIN are ORed to produce a common strobe. This strobe is
further qualified by the octal comparitor U51 if the bus address lines A1-A7 match the port address
selected by SW2, and the bus status is I/O. The decoder U40 distributes the combined board select
and strobe signal into two write strobes and two read strobes using the least significant bus
address line A0 and the I/O status.

Data from the CPU is buffered onto the board by U36 which is a latch, but always in its transparent
mode. This internal data bus goes to all of the software setable registers, namely CTL (U35),
CYLINDER (U55), HEAD (U54), SECTOR (U56) and DRIVE (U38). The CTL register is strobed directly by
one of the two write strobes derived above while the data registers CYLINDER, HEAD, SECTOR and DRIVE
receive their strobes from a second decoder, also in U40, which distributes the second write strobe
according to the two least significant bits of the operation code.

The first of the two read strobes enables the STATUS register (U37) to drive its contents directly
onto the CPU's data input bus. The second "read" strobe does not do a read at all. It drives the
STEP\* line in the disk interface cable through buffer U50.

| Register | Chip |
|---|---|
| CTL | U35 |
| STATUS | U37 |
| DRIVE | U38 |
| HEAD | U54 |
| CYLINDER | U55 |
| SECTOR | U56 |
| Port compare (A1-A7 vs SW2) | U51 |
| Strobe decode | U40 |
| Data-in buffer | U36 |
| STEP\* buffer | U50 |

### On the bus side: the DMA interface (pages 11-12)

The DISK 2 performs all data transfers between the disk and memory directly as opposed to techniques
in which the CPU would read or write a byte of data at a time from the controller and then write or
read the byte as appropriate to main memory. While CPU data transfer worked perfectly well for
floppy disk transfer rates, the byte rate of a hard disk does not allow nearly enough time for the
bus cycles required to do the two data transfers and fetch the required instructions. One
alternative to the speed problem is to read the data to a RAM buffer on the controller and then,
after the disk transfer has completed, the CPU could transfer data between the buffer and memory.
While this technique is sometimes used, most applications requiring a high performance disk are
better served by eliminating the need for the CPU to transfer the data at all, not just postponing
it.

DMA operations are described in the IEEE 696/S-100 specifications as temporary master operations
because the controller will temporarily take over as bus master generating all bus control signals
normally provided by the CPU and transferring data to memory on the CPU's data output bus or
receiving them from memory on the data input bus. The DISK 2 performs these operations in
conjunction with the SELECTOR CHANNEL. While the SELECTOR CHANNEL is physically a second board, the
two boards are required to work together to provide the temporary master as described in the
specifications.

When the finite state machine anticipates a requirement to access memory, it will assert the signal
K4. The gate U34 will wait for the bus hold and hold acknowledge signals HOLD\* and HLDA to both be
logically false (indicating that no other temporary master is currently using the bus) and then set
APRIO, the assert priority latch, starting the bus arbitration process. APRIO asserts the bus HOLD\*
line and enables the system of gates consisting of U43, U44 and U45 to drive the DMA priority set by
switches SW2 positions 2-5 **[sic]** onto the DMA arbitration lines DMA0\* - DMA3\*. The function of
the gates is to disable less significant bits of the priority if another device is asserting a
higher priority. If any of the bits are disabled, the IMHI (I'm high) will be false. By the time
HLDA is received from the CPU, IMHI will have settled. If the arbitration was lost (another device
of higher priority wanted the bus at the same time) U15 will clear APRIO. If this occurs, APRIO will
set again for another try as soon as the bus is available.

> Conversion note: the priority switches are **SW1** positions 2-5 (page 2); SW2 is the port
> address.

Meanwhile, over in the SC (SELECTOR CHANNEL), a circuit monitoring the arbitration and comparing the
winning number with the contents of its priority register will observe the DISK 2 winning the bus,
and perform the required exchange of bus mastership with the CPU. The DISK 2, on finding that the SC
had disabled the CPU status lines will set a latch (U18) which in turn will enable the DISK 2's
driver for the bus control line pSYNC. This completes the process of obtaining the bus. Nothing else
will happen until it is time to start a cycle.

When the finite state machine wishes to transfer a byte, it will set the DRQ flip-flop (U16). U17
will allow the first stage of U20 to clear at the next rising edge of the bus clock. The output of
this stage will enable pSYNC via U20. At the following bus clock, the same output will cause the
next stage of U20 to clear. The output of this stage will negate pSYNC also via U20. In response to
the pSYNC, the SC will generate the strobes required for the transfer, extending them as required to
satisfy any wait requests. U20 will latch the occurrence of the strobe in the third stage of U20 and
use the fact that this stage is set and the absence of strobes to clear DRQ with U17. This process
of setting DRQ, generating a pSYNC and clearing DRQ will repeat for each byte to be transferred
between the DISK 2 and memory.

When the finite state machine determines that it will no longer require access to the bus, it will
negate Q4. This will clear APRIO disabling the assertion of HOLD\* and the priority
lines. This is the signal for the SC to tranfers the mastership of the bus back to the CPU. The
latch U18 will assure that the DISK 2 will drive the pSYNC line until the SC releases the status
disable line SDSB\*.

> Conversion note: the same signal is called K4 at the start of this description and Q4 here. The
> manual uses both names for the state bits: K0-K4 in the text of pages 15-16, "Q4-Q0" and "Qn" in
> Figures 1 and 2. K4/Q4 is the top bit of the state number.

So the division of work in a DMA byte is (summary, not the manual's words): the DISK 2 arbitrates
(HOLD\*, DMA0\*-DMA3\*) and drives **pSYNC** once per byte; the Selector Channel holds the address,
drives the status and strobes, adds waits, and hands the bus back to the CPU when the DISK 2 drops
HOLD\*.

### On the disk side: the disk interface (page 13)

The DISK 2 has five connectors along its upper edge for the connection of drives. The four twenty pin
connectors J1 - J4 carry high speed differential data and clock signals between the DISK 2 and the
corresponding drive. J5 carries single ended open collector control and status lines between the
controller and all of the connected drives.

Four of the signals in this connector, DRIVE-SEL1\* through DRIVE-SEL4\* enable one of the connected
drives to use the balance of the lines which are multiplexed. The select lines are driven from the
upper nibble of the DRIVE register (U38) via the buffers U22 and U23. The lower nibble of the DRIVE
register drives HEAD-SEL1\* through HEAD-SEL4\* also using buffers in U22 and U23. In a given
application, less than four drives may be connected and the drives may have fewer than sixteen
heads. In addition, the STEP\* control line which was pulsed by reading the DISK 2 data port as
described above, the controller drives four other control lines. DIRECTION\* which controls the
direction during stepping and FAULT-CLR\* which is used to reset a drive WRITE-FAULT status are
driven from the OP2 and FAULT-CLR bits in the CTL register through buffers U22 and U23. The last two
control output lines READ-GATE\* and WRITE-GATE\* are generated by the finite state machine (to be
described below) and share the buffers in U22 and U23. Five input lines from the control cable are
terminated and used on the DISK 2. They are DRIVE-READY\*, TRACK0\*, WRITE-FAULT\*, INDEX\* and
SECTOR-CLK\*.

Each of the signal connectors on J1-J4 have a pair of differential line drivers and a pair of
differential line receivers associated with it. The line drivers send the differential signals
WRITE-CLOCK and WRITE-DATA to the disk. Each differential signal has a + and a - phased signal line
associated with it. The WRITE-DATA and WRITE-CLOCK for all of the drives are driven from the common
internal single ended signals WRITE-DATA and SEL-CLK. The line receivers receive the differential
signals READ-DATA and PLO-CLK from each of the drives. The READ-DATA signals are converted to single
ended signals RD1\* - RD4\* by the line receivers. RD1\* - RD4\* are then ORed by U14 to provide the
combined READ-DATA. The PLO-CLK signals from the drives are converted to their single ended
equivalents CK1 - CK4 by their line receivers. The CTL register bits US1 and US0 (for Unit Select)
are used by the multiplexor U13 to select one of these signals to be the internal clock. The signal
SEL-CLK is jumper selected from either the internal clock or its complement thus returning the
PLO-CLK received from the drive back to the drive as a WRITE-CLOCK. This is required because the
time delay of the round trip would introduce enough skew to make the interpretation of the
WRITE-DATA at the drive unreliable if the data were clocked in with the source of PLO-CLK. The
selection of internal CLOCK or CLOCK\* is required because of an inconsistency in some drives. The
signal cables also contain a single ended signal SEEK-COMPLETE\*. The lines from the four drives are
terminated and multiplexed again by U13 to provide SEEK-COMPLETE\*.

| Connector | Signals |
|---|---|
| J1-J4 (20-pin, one per drive) | differential WRITE-CLOCK, WRITE-DATA (out); READ-DATA, PLO-CLK (in); single-ended SEEK-COMPLETE\* (in) |
| J5 (50-pin, daisy chain, open collector) | out: DRIVE-SEL1\*-4\*, HEAD-SEL1\*-4\*, STEP\*, DIRECTION\*, FAULT-CLR\*, READ-GATE\*, WRITE-GATE\*; in: DRIVE-READY\*, TRACK0\*, WRITE-FAULT\*, INDEX\*, SECTOR-CLK\* |

> Note (interpretation): this explains why US1/US0 in the CTL register must match the DRIVE
> register (page 8). The DRIVE register picks the drive on the J5 cable; US1/US0 pick which J1-J4
> data cable supplies the clock, read data and SEEK COMPLETE. The SEL-CLK jumper is the J6
> "C / C\*" write clock phase jumper of page 3.

### In the middle: the disk data (pages 14-15)

Data is tranferred between the controller and disk in a bit serial format. The bit rates vary with
the type of drive but are in the order of 5 MHz. The READ-DATA signal is delayed by one bit time in
U11 to become READ-DLY and presented to the input of an octal shift register U58. Bytes of data are
available in parallel at the outputs of the shift register every eight bit times. The broadside
output of the shift register connects to the controllers internal shift register bus, the SR bus.
During a disk read, the bytes are captured from the SR bus by the octal register U52 which then
drives the data onto the computer's data output bus lines. U52 is clocked by the output of U11 which
will have a rising edge in the center of the last bit time.

The READ-DLY signal is delayed one more bit time by U24 to provide READ-2-DLY\*. This signal is ORed
in U25-3 with a signal from U25-6 which will be false during a read. The READ-2-DLY is then shifted
into a sixteen bit shift register formed by the two octal parts U41 and U42. This shift register has
exclusive ORed feedback from its last bit to its first and two points in between, the specific
connection of which results in the register accumulating the CRC check code for the read data. The
control circuitry must compare the CRC computed as the data is read with the CRC code read from the
disk immediately following the data. If the CRC computed during the read matches the CRC computed
and stored during the write operation, the data is, in all probability, correct. The calculated CRC
is available at the output of U42-12. This signal is connected to the left shift serial input of the
shift register U58, so that the CRC may be brought into the last bit of the shift register by
reversing the direction of shifting. This bit of the shift register is available on the Qh output
and called WRITE-DATA. The output of U26-3 is the exclusive OR of WRITE-DATA (the computed CRC) and
READ-2-DLY (the CRC read from the disk). A non-compare may be latched by U24, the SET-NE flip-flop.

> Note (interpretation): feedback from the last bit to the first and two points in between is the
> shape of the CRC-16 / CCITT polynomials (x^16 + x^12 + x^5 + 1 or x^16 + x^15 + x^2 + 1). The
> manual does not say which; the logic diagram (U41/U42 taps) would show it.

Writing the disk uses much of the same hardware as read. Data read from memory on the data input bus
is latched in U53 and held there until the end of a byte time of the hard disk at which point it is
parallel loaded by the shift register from the SR bus. The serial output of the shift register,
WRITE-DATA, is sent to the disk. During a write operation, U25-6 is enabled while all of the
READ-DATA signals are zero so that the CRC register will be computing the CRC code for the write
data. After all of the data has been written, the direction of the shift register is reversed
causing the CRC to be shifted into the left shift input and out to the disk on the WRITE-DATA line.
Besides writing data and CRC codes, the controller must also be capable of writing preambles
(sequences of zeroes) and mark characters (in this case, an FFh). These constants are available on
the SR bus from U57.

In addition to all of the above, the controller must identify the header field for the desired
record before any data is read or written. The three bytes of header data to be matched are stored
in the data registers CYLINDER, HEAD and SECTOR. The contents of these
registers may be gated onto the SR bus. During a read, the contents of a data register may be
compared with data coming from the disk using the same hardware which is used for checking the CRC.
The data register information is parallel loaded into the shift register at the start of a byte
time. As the read data is shifting through READ-2-DLY, the shift register is shifted to the right,
bringing the bits of the data word one at a time into the WRITE-DATA position. The two are compared
and any non-compare is latched in SET-NE.

### The finite state machine (pages 15-16)

While all of the preceding discussion about reading and writing the disk discussed the capabilities
of the shift register, the various sources of the SR bus, the CRC generator, the compare logic and
the DMA data input and output registers, no mention of how and when any of these facilities are
invoked was mentioned. The source for all of these control signals is a finite state machine (FSM)
implemented with bipolar PROMs and latches. At the end of every byte time of the hard disk, a
sixteen bit wide control word is read from the PROMs U47 & U48 into a register U30 and U31. The
contents of these registers will control all of the other hardware for the next byte time as well
as influencing which word will be read from the PROM next. The PROM address is made up of three
components. The first is the operation code which is obtained from the OP2 - OP0 bits of the CTL
register allowing up to eight instructions. The next field consists of K4 - K0, the current state,
allowing a maximum of thirty two states. The last field is only one bit, the signal ALT. ALT allows
the selection of one of two possible next states which shall be called normal and alternate. Thus
every cycle of the machine is a conditional branch. There are sixteen bits of PROM output. One field
of five bits contains the next state which become the current state K4 - K0 as it is loaded into the
latch. Another field is latched to produce SA2 - SA0. This is the SR bus address and selects which
of all of the SR bus sources discussed above will actually drive the bus during the next byte time.
The SR bus address lines are decoded by (U39). The next field contain NEXTA - NEXTC. These are the
address lines to the multiplexor U25 **[sic]** which selects which condition will generate ALT for
the current cycle. There are four additional signals in the state vector. CRC\* and LOAD control the
operation of the shift register. The shift register normally shifts one bit to the right every bit
time. If LOAD is true, the shift register will parallel load instead of shift in each bit 7 time. If
CRC\* is asserted, the shift register will shift to the left every bit time. READ and WRITE are
buffered to become READ-GATE\* and WRITE-GATE\*. These are disk control signals which will be
asserted just prior to the start of the next byte.

U33 is a four stage shift register which divides CLK by eight to form byte times. Its outputs, each
of which is available in both true and complement form, make available a unique rising and falling
edge for each bit time. One output, the signal Φa\*, provides a rising edge used to clock the state
register consisting of U30 and U31. This loads the control vector for the following byte time.

In the state register, bits K0 - K4 contain the present state number. NEXTA - NEXTC contain the
branch condition which is used in selection of the following state. SA0 - SA2 contain the address of
the SR bus source for the current byte time. All four of the remaining bits are reclocked into U32
by Φd which occurs one bit time prior to the start of the next byte. Two of the resulting outputs
are READ and WRITE, the signals which control READ-GATE\* and WRITE-GATE\*. The other two lines are
LOAD and CRC\*. U34 qualifies LOAD with Φa and Φd. This will force the S1 control input of the shift
register to be asserted during bit 7 if LOAD is true resulting in the shift register doing a
parallel load at the start of the next byte time. The signal CRC\* asserts S1 while negating S0
causing the shift register to reverse its direction for an entire byte time if CRC\* is true.

The inputs of the STATE register all come from the outputs of a pair of PROMs U47 and U48. The
address in the PROMs consists of three parts. The first field is OP2 - OP0, the current command. The
next is K0 - K4, the current state number. The last input is a signal called ALT for alternate. Thus
for every state of every command one of two exits is selected by the state of ALT. ALT originates
at the output of U29, a multiplexor which selects one of eight conditions based on NEXTA - NEXTC.
Its output, SET-ALT is reclocked by U32 to provide ALT. In designing the PROM, many states repeat for
fixed numbers of byte times. This kind of action is attained by having a state transition to itself
if ALT is false and advancing to the next state if ALT is true. Most of the ALT multiplexor inputs
are of this form. These include N1, N2, N4, N8 and NX. N1 is always true. N2 - N8 allow states with
the implied durations. NX is switch selected to one of N128, N256, N512, N1024 or N2048 depending on
the disk formatting. The remaining multiplexor inputs are RUN, which allows 0 states to wait for the
machine to be turned on, NE which allows it to verify a header or CRC match, and SEC which allows it
to wait for the the next SECTOR\* from the disk. The byte times all come from the byte counter U27
and U28 which counts once each byte time due to Φb\* and is reset at the start of each new state by
ALT and Φa\* in U46. SEC is the SECTOR\* signal from the disk latched by U18 to hold it presence for
at least one byte time.

> Conversion note: page 15 calls the ALT multiplexor U25, page 16 calls it U29. The parts list
> (page 28) settles it: U29 is a 74LS151 (8-input multiplexer), U25 a 74LS00 (the gate U25-3 /
> U25-6 of page 14). U29 is right.

| PROM address bits | Meaning |
|---|---|
| OP2-OP0 | the command (up to 8) |
| K4-K0 | the current state (up to 32) |
| ALT | normal or alternate next state |

| PROM output (16 bits, latched in U30/U31, part reclocked in U32) | Meaning |
|---|---|
| next state (5 bits) | becomes K4-K0 |
| SA2-SA0 (3 bits) | SR bus source for the next byte time (decoded by U39) |
| NEXTA-NEXTC (3 bits) | which condition makes ALT |
| CRC\*, LOAD | shift register direction / parallel load |
| READ, WRITE | READ-GATE\*, WRITE-GATE\* |

| ALT condition | Meaning |
|---|---|
| N1 | always true (one byte time) |
| N2, N4, N8 | after 2, 4, 8 byte times |
| NX | after N128, N256, N512, N1024 or N2048 byte times, set by switch (the sector size, SW1-6..10) |
| RUN | wait for the CPU to start the machine |
| NE | header or CRC compare |
| SEC | the next SECTOR\* pulse |

That is 8 conditions, matching an 8-input multiplexor (the table is a summary, not the manual's).

The control software should verify a READY status from the selected drive before starting an
automatic operation for two reasons. First, the finite state machine is clocked by the disk and so
will hang if a non-existent or not ready disk is selected. The second is that the TIME-OUT mechanism
which is used to halt the machine if, for example, a header is not found uses INDEX\* from the
selected drive. Index clocks one of the counters in U27. This counter is held in a cleared state
until RUN and SEEK-COMPLETE are true by U46. If a operation fails to complete in two passes of the
index from the time it starts, TIME-OUT will be generated clearing and halting the machine.

### Track format and state timing (pages 17-18)

The PROM code implements the track formats/timing diagrams as shown below in Figure 1. This diagram
relates the various states of the four major operations to the position of the disk (i.e. relative
to sector pulses and recorded data). Note that all of the functions wait for the start of a sector.
READ HEADER or WRITE HEADER will proceed to completion on finding a header. READ DATA and WRITE DATA
will loop and wait for the next sector if the current sector header does not match the desired
cylinder, head and sector number and have a current CRC check code.

**Figure 1. DISK 2: Track format and state timing** (a timing drawing; transcribed here).

One sector on the disk, starting at the SECTOR pulse from the drive:

| Field | Bytes | Contents |
|---|---|---|
| Header preamble | 16 | 00H |
| Mark | 1 | FFH (`*` in the figure) |
| Header (HEAD) | 3 | cylinder, head, sector (one byte each) |
| Header CRC | 2 | check code |
| Header postamble | 2 | 00H |
| *write splice* | | (WRITE DATA starts writing here) |
| Data preamble | 16 | 00H |
| Mark | 1 | FFH |
| Data | sector size (128-2048, SW1-6..10) | the sector |
| Data CRC | 2 | check code |
| Data postamble | 2 | 00H |

Figure notes, as printed: "Numbers are states (Q4-Q0). \* is one byte of FFh. HEAD is CYL, HEAD,
SECT (each one byte). CRC is check code (2 bytes). **Format overhead 45 bytes.**" The fields above
add up to 45 bytes plus the data, which agrees with the note.

Gate signals drawn against that layout:

| Signal | Active from | to |
|---|---|---|
| WRITE (HEAD) | start of the header preamble | the write splice |
| READ (HEAD) | part way into the header preamble | end of the header CRC |
| HANG (HEAD) | later in the header preamble | end of the header CRC |
| WRITE (DATA) | the write splice | end of the data postamble |
| READ (DATA) | part way into the data preamble | end of the data CRC |
| HANG (DATA) | later in the data preamble | end of the data CRC |

State sequences (state numbers along the time axis, one box per state):

| Operation | States |
|---|---|
| WRITE DATA | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 (header pass), then 16, 17, 18, 19 (data), 20, 31 |
| READ DATA | 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 (header pass), then 16, 17, 18, 19 (data), 20, 31 |
| WRITE HEADER | 1, 16, 17, 18, 19, 20, 21, 22, 31 |
| READ HEADER | 1, 16, 17, 18, 19, 20, 21, 31 |

> Note (interpretation): "HANG" is not explained in the text. From its place in the drawing (it
> starts inside the preamble and ends with the CRC, after READ has started) it is probably the
> point where the controller locks onto the data and waits for the FFH mark. The state numbers
> show that READ DATA and WRITE DATA share the header-search states 1-10.

Not all signals change state on byte boundaries. Figure 2 shows the timing of various signals
relative to the clock phases. Note that the state responsible for a given action is generally about
one byte ahead of the data on the disk. This is because writing a byte of data to the disk or
checking a byte of data read from the disk requires that data be read from a register during the
preceding byte time and loaded into the shift register at the very start of the following byte
time. In the singular case of reading data from disk to memory, the command in the current byte
causes the byte of data read during the following byte to be enabled onto the SR bus from which it
will be captured just prior to the start of the third byte by the DO register. During the third byte
time, the data will be sent to memory.

**Figure 2. DISK 2: Timing of various signal relative to the clock phases** (page 18; a timing
drawing, described, not transcribed). Three byte times are drawn. ΦA-ΦD are four phase-shifted
square waves (the U33 outputs), each one bit time apart. The state outputs Qn, NEXTn, SAn change at
the byte boundary; READ, WRITE, LOAD, CRC\*, ALT, HANG change one bit time earlier (Φd, as page 16
says). Arrows show the chain SR NEXT\* -> SR ENA\* -> (next byte) DO ENA\* -> DO CLK, so a byte read
from the disk reaches the DO register two byte boundaries after its command. Also drawn: SR LOAD
(a short pulse just before the boundary), BYTE COUNTER CLEAR, BYTE COUNTER CLOCK, CLR NE\*, and CLK
NE (one pulse per bit for eight bits) with the NE result valid at the end of the byte.

## Hardware section (pages 19-29)

- Page 19 (PDF 23): section title page, "HARDWARE SECTION".
- Pages 20-27 (PDF 24-31): **logic diagram**, 8 sheets, each marked "DISK 2 177D ©1982" and
  "1 OF 8" ... "8 OF 8". Not transcribed (use the PDF). The contents page calls them pages 20-27.
  Sheet 8 carries the 50-pin J5 pin numbers along its top edge.
- Page 28 (PDF 32): parts list, below.
- Page 29 (PDF 33): component layout (the board outline with numbered chip positions, marked
  "DISK 2"). Not transcribed.

### Parts list (page 28)

| Unit | Part | | Unit | Part |
|---|---|---|---|---|
| U1 | 7805 | | U33 | 74LS175 |
| U2 | 7805 | | U34 | 74LS10 |
| U3 | 9638 | | U35 | 74LS273 |
| U4 | 9637 | | U36 | 74LS373 |
| U5 | 9638 | | U37 | 74LS373 |
| U6 | 9637 | | U38 | 74LS273 |
| U7 | 9638 | | U39 | 74LS138 |
| U8 | 9637 | | U40 | 74LS139 |
| U9 | 9638 | | U41 | 74LS273 |
| U10 | 9637 | | U42 | 74LS273 |
| U11 | 74LS74 | | U43 | 74LS32 |
| U12 | 74LS00 | | U44 | 74LS08 |
| U13 | 74LS153 | | U45 | 74LS38 |
| U14 | 74LS20 | | U46 | 74LS00 |
| U15 | 74LS32 | | U47 | 177P-1 (state machine PROM) |
| U16 | 74LS74 | | U48 | 177P-2 (state machine PROM) |
| U17 | 74LS00 | | U49 | 74LS273 |
| U18 | 74LS279 | | U50 | 74LS125 |
| U19 | 74LS04 | | U51 | 25LS2521 |
| U20 | 74LS175 | | U52 | 74LS374 |
| U21 | 74LS02 | | U53 | 74LS373 |
| U22 | 7406/16 | | U54 | 74LS374 |
| U23 | 7406/16 | | U55 | 74LS374 |
| U24 | 74LS74 | | U56 | 74LS374 |
| U25 | 74LS00 | | U57 | 74LS373 |
| U26 | 74LS86 | | U58 | 74LS299 |
| U27 | 74LS393 | | Q1 | 2N2222 |
| U28 | 74LS393 | | D1 | 1N747 |
| U29 | 74LS151 | | | |
| U30 | 74LS273 | | | |
| U31 | 74LS273 | | | |
| U32 | 74LS273 | | | |

The "(state machine PROM)" labels on U47/U48 are from page 15, not the parts list. U3-U10 are
four 9638 + 9637 pairs; with page 13 this reads as one dual line driver (9638: WRITE-CLOCK,
WRITE-DATA) and one dual line receiver (9637: READ-DATA, PLO-CLK) per drive connector J1-J4
(interpretation; the pairing per connector is not printed).

| Resistor | Value |
|---|---|
| R1,2 | 4.7K Ohm |
| R3,4 | 110 Ohm |
| R5,6 | 4.7K Ohm |
| R7,8 | 110 Ohm |
| R9,10 | 4.7K Ohm |
| R11,12 | 110 Ohm |
| R13,14 | 4.7K Ohm |
| R15,16 | 110 Ohm |
| R17-19 | 330 Ohm |
| R20-24 | 220 Ohm |
| R25-27 | 330 Ohm |
| R28 | 68 Ohm |
| R29-36 | 1K Ohm |
| SR1 | 2.7K-5 SIP |
| SR2 | 2.7K-5 SIP |

> Conversion note: the theory of operation also names U43, U44 and U45 as the DMA priority gates
> and U51 as the "octal comparitor"; the parts list agrees (74LS32, 74LS08, 74LS38; 25LS2521 is an
> 8-bit comparator).

# Selector Channel section

The Selector Channel has its own chip numbers (U1-U30); a "U25" here is not the DISK 2's U25.

## About Selector Channel (page 31)

The SELECTOR CHANNEL is an IEEE 696/S-100 compatible board which is designed to provide all of the
bus address, status and control strobes required by a temporary master (DMA device) designed to use
it. The combination of a selector channel and companion device controller are the functionally
equivalent to a single board temporary master. The hardware on the SELECTOR CHANNEL however may be
shared by a number of device controllers.

## Technical overview (page 31)

The SELECTOR CHANNEL has a single I/O port through which the CPU or other bus master may program the
identification of the device controller to be supported as well as the addresses of and types of
cycles to be generated. When the selected device is granted access to the bus, the SELECTOR CHANNEL
will exchange mastership of all of the address, status and control lines with the CPU and generate
cycles as requested by the device controller. In addition, the SELECTOR CHANNEL has a BOOT function
which may be used to initialize the system from one of the devices whose controllers share the
SELECTOR CHANNEL.

## Switch settings and option selection (page 31)

The SELECTOR CHANNEL's I/O port may be addressed anywhere in the 00h - FFh range using switches
SW1-3 through SW1-10. SW1-1 is used to enable the on board BOOT EPROM. SW1-2 is used to enable one
wait state minimum for all DMA cycles generated by the SELECTOR CHANNEL. Wait states may also be
requested in software when the SELECTOR CHANNEL is programmed and the SELECTOR CHANNEL will honor
wait states requested by bus slaves during temporary master cycles which is supports.

In addition to Switch 1, the SELECTOR CHANNEL has a number of options which are controlled by the
installation of push-on jumpers. J1 - J4 enable one through four wait states respectively for the on
board EPROM. As the SELECTOR CHANNEL is shipped, J4 is connected by a trace to allow maximum access
for slow EPROMS. If fewer EPROM wait states are required, this trace may be cut and a jumper may be
installed in one of the other locations. These waits are only generated when initially booting a
system with the EPROM, so the insertions of waits will not adversely affect system operation.

The SELECTOR CHANNEL uses a 2716 type EPROM which has a capacity of 2 K bytes. Since this capacity is
greater than required for most boot operations, jumpering is provided which will allow the space to
be divided into several smaller code segments and the selection of one of them. This will enable a
single, standard EPROM to hold boot code for a number of device controllers and configurations.
Jumper locations J8 through J10 allow the corresponding address lines of the EPROM to be tied to the
corresponding bus address lines providing a larger address range or to be tied high and low in any
combination selecting a section of code within the 2716.

| Switch / jumper | Function |
|---|---|
| SW1-1 | enable the on-board BOOT EPROM |
| SW1-2 | at least one wait state on every DMA cycle |
| SW1-3 ... SW1-10 | I/O port address (00H-FFH) |
| J1-J4 | 1-4 wait states for the EPROM (J4 by a trace as shipped) |
| J8-J10 | EPROM address lines: to the bus, or tied high/low to pick a code section |

> Conversion note: the text gives no table of which SW1 position is which address bit, and no
> port value. **Logic sheet 1 (page 36)** shows it: SW1-3 ... SW1-10 go to the U26 comparator
> against A7 ... A0 (S-100 pins 83, 82, 29, 30, 31, 81, 80, 79), each with a SIP pull-up and the
> switch to ground, so **ON = 0, OFF = 1**:
>
> | SW1 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
> |---|---|---|---|---|---|---|---|---|
> | Address bit | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0 |
> | For port F0H | OFF | OFF | OFF | OFF | ON | ON | ON | ON |
>
> The CompuPro software uses port **F0H** (`SELCHAN EQU 0F0h` in the CP/M 8-16 V1.1R
> `COMPUPRO.EQU`), and the MP/M 8-16 Technical Manual's Selector Channel settings
> (p. 36) are exactly the F0H row, with SW1-1 OFF (no boot EPROM) and SW1-2 ON.

## Programming the Selector Channel (pages 32-33)

The first step in programming the SELECTOR CHANNEL is to do a read from its I/O port. The read
operation will disable the SELECTOR CHANNEL during the following four write cycles preventing it
from supporting any temporary master cycles while it has a partial program. No data is returned by
the read. The SELECTOR CHANNEL powers up in this disabled state.

The SELECTOR CHANNEL must then be sent a sequence of exactly four bytes. They are as follows:

| Order | Byte | Meaning |
|---|---|---|
| 1 | A23-A16 | The extended address byte |
| 2 | A15-A8 | The high address byte |
| 3 | A7-A0 | The low address byte |
| 4 | MODE | The mode control byte |

Three address bytes are required because the SELECTOR CHANNEL addresses the full 24 bit extended
address range of the IEEE 696/S-100 bus. An extended address byte must be sent even if the memory or
I/O devices which will be addressed are going ignore its contents. Example: to send address 0FFFF0
the bytes in order would be 0F, FF, and F0. The mode byte is made up of two parts. The bits in the
upper nibble control the mode as follows:

| Bit | Name | Function |
|---|---|---|
| D7 | W/R\* | Write / Read relative to the bus slave (memory or I/O port). |
| D6 | IO/M\* | IO / Memory - controls the status of cycles generated. Also, memory addresses are incremented on multi-byte transfers. I/O addresses are not. |
| D5 | U/D\* | Up / Down - A memory address may be counted up or down after each byte. |
| D4 | WAIT | WAIT - Imposes a single wait state for each bus cycles supported. |

The lower nibble of the MODE byte contains the identification of the controller to be supported.
This nibble should contain the complement of the device priority.

| Bit | Name | Function |
|---|---|---|
| D3 | DMA 3\* | Select complement of DMA device that Selector Channel will work with. |
| D2 | DMA 2\* | |
| D1 | DMA 1\* | |
| D0 | DMA 0\* | |

Once the SELECTOR CHANNEL has been programmed, it will respond to all cycles requested by the
selected device. The SELECTOR CHANNEL function is independent of whether the device does individual
cycles or burst mode transfers. The SELECTOR CHANNEL may also support a sequence of burst mode
transfers to successive locations without reprogramming. If a controller function for which the
SELECTOR CHANNEL may have transferred one or more bytes of data is going to be repeated, the
SELECTOR CHANNEL must be reprogrammed. The SELECTOR CHANNEL may be disabled at any time by reading
its I/O port.

> Example from the CompuPro driver (CP/M 8-16 V1.1R, not part of this manual): `SELPRI EQU 10` (the
> DISK 2 priority, SW1-2..5) and `SELBYT EQU 2Fh-SELPRI` = **25H**: U/D\* = 1 (count up),
> IO/M\* = 0 (memory), WAIT = 0, low nibble 0101B = the complement of 10 (1010B). `TMXDISK2.DVR`
> does `IN AL,SELCHAN`, sends the three address bytes high to low, then the mode byte: **A5H** for
> a disk read (W/R\* = 1, the DMA writes to memory) and **25H** for a disk write. Only then does it
> write the command to the DISK 2 CTL port.

## Theory of operation (pages 33-34)

The octal comparator U26 decodes the SELECTOR CHANNEL's I/O port address qualified by I/O status to
provide BD-SEL\*. The bus reset signal RESET\* or BD-SEL\* in conjunction with the read strobe pDBIN
will disable the SELECTOR CHANNEL by clearing the counter U10. U10 will count write cycles since it
is clocked by BD-SEL\* and pWR\*, the bus write strobe. After four write cycles, CLEAR\* which is
obtained from the Qd output of U10 will hang U10 by disqualifying its clock and enable the SELECTOR
CHANNEL to respond to the selected controller.

All data sent to the SELECTOR CHANNEL is loaded into the MODE register U17. The output of U17 is
connected in parallel to the least significant byte of address counter formed by the four bit
counters U6 and U16. Similarly, the output of the low byte of the counter provides the input to the
high byte counter U3 and U13 which in turn provides the input to the extended byte counter U4 and
U14. Thus the MODE register and three bytes of address counter form a four stage, eight bit wide
shift register. The counters are set to load rather than count whenever the SELECTOR CHANNEL is not
actually involved in a temporary master cycle. A LD-STB derived from BD-SEL\* and pWR\* clocks the
address counters as well the MODE register causing the shifts. The low, high and extended address
counters have associated bus drivers U25, U22 and U23 respectively, which are enabled during master
cycles.

> Note (interpretation): this shift-register design is why the byte order is fixed and why exactly
> four writes are needed: each write shifts MODE -> low -> high -> extended, so the first byte sent
> (A23-A16) ends in the extended counter and the last (MODE) stays in U17. It also means the 24-bit
> counter carries from low to high to extended byte.

The IO/M\* and W/R\* status bits of the MODE register are decoded by U7 along with the generalized
strobes. The resulting status is driven onto the bus by U18 during master cycles while the strobes
are output by U24.

The XFER1 flip-flop (U5) is used to detect that the selected controller has obtained the bus. The
flip-flop will set at the first falling edge of the bus clock following the combination of HOLD\*,
HLDA and IMHI all being true. IMHI indicates a match between the priority of the device holding the
bus and the target device whose priority number is stored in the low nibble of the MODE register.
When XFER1 sets, it indicates the start of the overlap period during which the permanent and
temporary masters both drive the command lines. If XFER1 is set, XFER2 in U12 will set at the next
rising edge of the clock. XFER2 enables the balance of the bus drivers and disables the remainder of
the masters bus drivers. Once XFER2 has set, the SELECTOR CHANNEL will wait for cycles to be
requested by the controller.

When the device controller is ready to transfer a byte of data, it will generate the bus signal
pSYNC. When the SELECTOR CHANNEL detects pSYNC, it will start a bus cycle by generating a pSTVAL\*
signal when pSYNC is high and Φ **[as printed: "0"]** is low. The read or write strobe will be
asserted with the next rising edge of Φ and stay asserted for one clock cycle unless a DMA wait or
external wait is requested by having a WRQ signal asserted. The strobe will go away one clock cycle
after the WRQ signal goes low. The BC signal will be asserted on the next clock after the read or
write strobe is asserted and then go away after the strobe is released. The falling edge of BC will
step the address counters to the next DMA address for the next data transfer.

> Conversion note: the scan prints "0" for the clock; it is read here as the bus clock Φ.

When the controller is finished with the bus, it will remove the signal HOLD\*. The following rising
edge of the clock will remove XFER2 and the negative edge after that will remove XFER1.

The on board boot EPROM may share address space with RAM memory which responds to PHANTOM\*. The
BOOT latch (U30) is enabled at RESET\* and will assert PHANTOM\* (if SW1-1 is ON) on all CPU memory
reads, disabling the RAM and enabling the on board EPROM. PHANTOM\* is not asserted during temporary
master operations, thus allowing data transfers between the device controller and RAM. The BOOT
latch will be reset the first time the CPU writes to RAM. The EPROM (U27) receives its addresses
directly from the bus. The data driver (U28) places it output on the data input bus during memory
reads when the boot is enabled. The counter U19 implements wait states by pulling RDY low for a
selected number of clock cycles of strobe.

## Hardware section (pages 35-41)

- Page 35 (PDF 39): section title page, "HARDWARE SECTION".
- Pages 36-39 (PDF 40-43): **logic diagram**, 4 sheets, "PAGE 1 OF 4" ... "PAGE 4 OF 4". Not
  transcribed (use the PDF); the port-address part of sheet 1 is in the switch note above. Sheets
  1-3 are marked "SELECTOR CHANNEL 172D ©1981"; **sheet 4 is marked "172B"** (probably an older
  sheet left in; the manual does not say). Sheet 4 has the EPROM
  address jumpers with positions R, G and + (see the MP/M J8-J10 settings: "G" and "+").
- Page 40 (PDF 44): parts list, below.
- Page 41 (PDF 45): component layout, a photo of a board marked "SELECTOR CHANNEL 172C", captioned
  "NO COMPONENT CHANGES BETWEEN REVISIONS C AND D". Not transcribed.

> Note (interpretation): the R / G / + positions read as "to the bus address line", "to ground"
> and "to +5 V", which matches page 31 (tied to the bus, or tied high or low). The manual does not
> spell the letters out.

### Parts list (page 40)

| Unit | Part | | Unit | Part |
|---|---|---|---|---|
| U1 | 7805 | | U20 | 74LS38 |
| U2 | 74LS02 | | U21 | 74LS266 |
| U3 | 74LS169 | | U22 | 74LS244 |
| U4 | 74LS169 | | U23 | 74LS244 |
| U5 | 74LS74 | | U24 | 74LS244 |
| U6 | 74LS169 | | U25 | 74LS244 |
| U7 | 74LS139 | | U26 | 25LS2521 |
| U8 | 74LS00 | | U27 | 2716 (optional) |
| U9 | 74LS04 | | U28 | 74LS244 |
| U10 | 74LS93 | | U29 | 74LS02 |
| U11 | 74LS02 | | U30 | 74LS00 |
| U12 | 74LS175 | | | |
| U13 | 74LS169 | | | |
| U14 | 74LS169 | | | |
| U15 | 74LS10 | | | |
| U16 | 74LS169 | | | |
| U17 | 74LS273 | | | |
| U18 | 74LS240 | | | |
| U19 | 74LS175 | | | |

| Resistor | Value |
|---|---|
| R1 | 4.7K Ohm |
| R2 | 4.7K Ohm |
| R3 | 4.7K Ohm |
| SR1 | 5.1K Ohm |

The six 74LS169s (U3, U4, U6, U13, U14, U16) are the 24-bit up/down address counter of page 33, two
per byte; the 74LS169 is a 4-bit synchronous up/down counter with parallel load, which fits the
U/D\* mode bit. The boot EPROM U27 is optional.

# Customer service and warranty (PDF page 46)

Summary, not transcribed in full: contact your CompuPro dealer first. Products are sold Assembled
Tested (1-year limited warranty) or under the Certified System Component (CSC) program (200-hour
burn-in, 2-year limited warranty, direct exchange). For service, get an RMA number at
(415) 786-0909 and ship to CompuPro - RMA #, 3339 Arden Road, Hayward CA 94545. Warranty: 1 year
(2 years CSC, 6 months for disk drives) from purchase by the original end user; repair or replace at
CompuPro's option; written warranty address 3506 Breakwater Court, Hayward CA 94545. The usual
limitation of implied warranties and of incidental or consequential damages follows.

PDF page 47 is the back cover: "DISK 2 & SELECTOR CHANNEL", CompuPro, a Godbout Company, 3506
Breakwater Court, Hayward, CA 94545.

# Conversion notes worth knowing

Places where the manual disagrees with itself or with the hardware. Each is explained where it
appears.

| Topic | Where | Note |
|---|---|---|
| WRITE FAULT bit | p10 | Says bit 2 of STATUS; the p5 table, the p9 "status 3" rule and `COMPUPRO.EQU` all say bit 1. |
| Priority switches | p12 | Says "SW2 positions 2-5"; p2 says SW1-2..5 (SW2 is the port address). |
| ALT multiplexor | p15 | Says U25; p16 and the parts list say U29 (74LS151). U25 is a 74LS00. |
| K4 / Q4 | p12 | The same FSM signal has both names; the figures use Q, the text K. |
| "READ SELECTION" | p7 | The paragraph is about head selection. |
| Op Code Table | p8 | Referred to, not printed; rebuilt from the command numbers and checked against the source. |
| Selector Channel port bits | p31 | Not in the text; read from logic sheet 1 (SW1-3 = A7 ... SW1-10 = A0, ON = 0). |
| Selector Channel sheet 4 | p39 | Marked 172B; the other sheets are 172D. |
| Clock name | p34 | Printed "0"; read as Φ. |

# What this means for a DISK 2 model (interpretation)

Not the manual's words. Facts for the `disk2` and `selch` boards (`docs/boards/compupro-disk2.md`,
`compupro-selch.md`), written before they were built:

- **Two boards.** The DISK 2 (ports C8H/C9H) never addresses memory itself; the Selector Channel
  (port F0H) holds the 24-bit address and generates the bus cycles. A model can treat the pair as one
  DMA device but must keep the two programming interfaces: the IN F0H + 4 x OUT F0H sequence, and
  the DISK 2 CTL/DATA registers. A transfer with an unprogrammed (disabled) Selector Channel moves no
  data; on the real pair this probably ends in OVER RUN or TIME OUT (p9 points to the Selector
  Channel programming when OVER RUN persists; not tested).
- **Priority match.** The Selector Channel only serves the device whose DMA0\*-DMA3\* priority is the
  complement of its MODE low nibble. CompuPro uses priority 10 (`SELPRI`), so MODE = A5H (read) or
  25H (write). With TMA arbitration already in pascal100 (`dma_priority`), the DISK 2 gets
  `dma_priority = 10` by default, and the Selector Channel should refuse cycles for another priority.
- **Address counting.** Up or down per U/D\*, +1 per byte, carrying through all 24 bits; I/O cycles
  do not count. Not reset between commands: WRITE HEADER relies on the counter moving 3 bytes per
  sector across a track of commands (p10).
- **Registers.** CTL/STATUS bits (p5); data registers by OP1/OP0 with RUN = 0; DRIVE = one-hot drive
  select in bits 7-4 + head 0-15 in bits 3-0; US1/US0 must match the DRIVE register; a read of C9H is
  one STEP pulse in the OP2 direction (OP2 = 1 = in, per `D2_SIN`). STATUS is latched when ATTN\* is
  low (operation done) and transparent while ATTN\* = 1.
- **Commands** (RUN = 1, ATTN\* = 1): 0 NULL, 1 READ DATA, 2 WRITE DATA, 3 WRITE HEADER, 4 READ
  HEADER. Each waits for SEEK COMPLETE and then a SECTOR pulse; READ/WRITE DATA search for a header
  matching CYLINDER/HEAD/SECTOR with a good CRC and give TIME OUT after two index pulses. NULL ends
  at index (or TIME OUT). A good read leaves STATUS = 03H (02H on track 0). ATTN comes up asserted
  after RESET (p3).
- **Image format.** Each sector on the disk carries a 3-byte header that software wrote with WRITE
  HEADER; the hardware does not care what it holds (p10), though CompuPro writes cylinder, head and
  sector. A plain image (sectors only) is enough if the model assumes the standard headers and
  fakes READ HEADER from the position; a faithful model of formatting would have to store the
  headers too. Per-sector overhead on the real disk is 45 bytes (Figure 1), which matters only for
  timing.
- **Fujitsu M20 (the DISK2 default):** 1024-byte sectors (SW1-7 ON), skew 2. The drive is the
  Fujitsu **M2302B** (and `m10` the **M2301B**); its CE manual
  (*M2301B/M2302B Microdisk Drives CE Manual*, Fujitsu, B03P-4605-0003A rev B, spec table p. 1-6) gives
  **244 cylinders**, 8 heads (M2301B: 4), 12,000 unformatted bytes per track, 2964 rpm, 593 KB/s,
  19.98 MB formatted. Its Table 3.7 gives **11 sectors = 1090 bytes per sector** for SW1 keys 2 + 7
  and SW2 key 6 ON (64 + 2 + 1024), which is the drive setting on page 4 above; 1090 bytes holds a
  1024-byte sector plus the 45 bytes of Figure 1. So the M20 is 244 x 8 x 11 x 1024 =
  21,987,328 bytes, the same as the 2.1F XIOS20 DPBs (cylinder 0 reserved).
- **Boot.** The Selector Channel boot EPROM (SW1-1, PHANTOM\*, 2716 with J8-J10 segment jumpers) is
  optional and not used by the CompuPro MP/M setup (SW1-1 OFF); the MP/M hard-disk boot loads from a
  floppy (`MPMLDRHD.85` on the boot floppy reads MPM.SYS from the floppy).
- **Interrupts.** ATTN to VI0-VI7 (J7-J14) or INT (J15). CP/M 8-16's TMXDISK2 polls STATUS bit 7,
  but **MP/M 8-16 2.1E's `XIOS20.MPM` needs the interrupt on VI1 (J8)**: after the command it
  unmasks 8259 IR1 (`IN AL,51H / AND AL,0FDH / OUT 51H,AL`) and flag-waits (`INT 0E0H`, CL = 84H,
  flag 9); its interrupt routine masks IR1 and sets flag 9 (disassembled 2026-11-14). That is the
  MP/M manual's "Jumper setting: 8 -- Install a jumper": J8.
