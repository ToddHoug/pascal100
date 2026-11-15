# Western Digital UCSD Pascal III.0 — the P-machine and OS tables (Appendices C, H, I)

Source: [80-0130016-00A0_UCSD_Pascal_III.0_Operating_System_Reference_Manual_Jul82.pdf](#)
(Western Digital Corporation, *UCSD Pascal III.0 Operating System Reference Manual*, document
80-0130016-00A0, July 1982; 474 pp.; scan by bitsavers, no text layer)

This manual is the operating-system reference for the WD Pascal MicroEngine. Its **Appendix C,
Table C-3** describes every III.0 P-code operator in a Pascal-like metalanguage, with the record
declarations the MicroEngine operating system uses. It is the specification the `pascal100`
P-machine core is written from. This file transcribes Table C-3 (printed pages C-33 to C-50,
PDF pages 407-424, sections 1-3), Appendix H, the boot and initialization messages (section 4),
Appendix I, the III.0 OS globals of release H3 (section 5), and Table C-1, the opcode list
(printed C-1 to C-13, section 6).

Printed page C-*n* is PDF page *n* + 374. Pages are cited by their printed number.

Conventions of the source (C-33): `sp^.i` is the contents of the memory word that the top-of-stack
register points at, taken as an integer; `(sp+1)^.i` is the word above it. `i<x:y>` is the field
starting at bit position *x* for *y* bits. Addresses are **word** addresses. The stack **grows
toward low memory**: a push is `sp := sp - 1`.

Each operator is transcribed as printed, from the page image. Where the scan or the notation is
unclear, the line carries ⚠ and a note. Differences from the Digicomp Pascal-100 manual's
appendix F (`Digicomp Research Pascal-100.md`) are listed at the end.

---

## 1. Declarations (C-33, C-34)

```pascal
const
    version  = 'B0';   { Version of this document }
    mscw_sz  = 4;      { Size of mark stack control word in words }
    real_sz  = 2;      { Size of reals in words }
    bset_sz  = 4080;   { Max size of sets in bits }
    iset_sz  = 255;    { Max size of sets in words }
    word_sz  = 16;     { Size of word in bits }
    NIL      = -1024;  { Representation for nil pointer }
type
    object_type = (int_obj, real_obj, byte_obj, bool_obj, set_obj,
                   ptr_obj, sv_obj, sem_obj, mscw_obj, tib_obj);
    byte    = 0..255;
    sibp    = ^sib;
    sibvec  = array [0..127] of sibp;
    sib     = record { segment info block }
                segbase: memp;     { memory address of seg }
                segleng: integer;  { # words in segment }
                segrefs: integer;  { active calls }
                segaddr: integer;  { absolute disk address }
                segunit: integer;  { physical disk unit }
                prevsp : memp;     { SP saved by getseg for relseg }
              end { sib };
    mscwp   = ^mscw;
    mscw    = packed record { mark stack control word }
                msstat: mscwp;     { lexical parent pointer }
                msdynl: mscwp;     { ptr to caller's mscw }
                msipc:  integer;   { byte index in return code seg }
                msseg:  byte;      { seg # of caller code }
                msflag: byte
              end { mscw };
    tibp    = ^tib;
    tib     = packed record { Task Information Block }
                waitq:  tibp;      { QUEUE LINK FOR SEMAPHORES }
                prior:  byte;      { TASK'S CPU PRIORITY }
                flags:  byte;      { STATE FLAGS...reserved }
                splow:  memp;      { LOWER STACK POINTER LIMIT }
                spupr:  memp;      { UPPER LIMIT ON STACK }
                sp:     memp;      { ACTUAL TOP-OF-STACK POINTER }
                mp:     mscwp;     { ACTIVE PROCEDURE MSCW PTR }
                bp:     mscwp;     { BASE ADDRESSING ENVIRONMENT PTR }
                ipc:    integer;   { BYTE PTR IN CURRENT CODE SEG }
                segb:   memp;      { PTR TO SEG CURRENTLY RUNNING }
                hangp:  semp;      { WHICH TASK IS WAITING ON }
                iorslt: integer;   { Result of last I/O call. }
                sibs:   ^sibvec    { ARRAY OF SIBS FOR 128..255 }
              end { TIB };
    memp     = ^memtrix;
    register = memp;
    memtrix  = record
                 case object_type of
                   int_obj  : (i : integer);
                   bool_obj : (bool: boolean);
                   real_obj : (r : real);   { Standard, IEEE format }
                   byte_obj : (b : packed array [0..maxint-1] of byte);
                   set_obj  : (sz : integer;
                               sb : packed array [0..bset_sz-1] of boolean);
                   ptr_obj  : (p : memp);
                   mscw_obj : (m : mscw);
                   tib_obj  : (t : tib);
                   sv_obj   : (sv: sibvec);
                   sem_obj  : (count: integer;
                               waitq: tibp);
               end {record memtrix};
var
    pc,    { program counter }
    sp,    { points at top item on stack which grows toward low memory }
    mp,    { points at current mark stack control word }
    bp,    { points at global mark stack control word }
    segb,  { points at base of currently executing code segment }
    ctp,   { points at TIB for currently executing task }
    rq,    { points at list of TIB's for ready to run tasks }
    ssv:   { points at system segment vector }
        register;

    lm,
    lsv,
    src,
    dst:   memp; { General use temporaries }
```

Notes on the declarations:

- `mscw` is 4 words (`mscw_sz`): `msstat`, `msdynl`, `msipc`, then `msseg` and `msflag` packed
  into the fourth word. Which byte of that word holds `msseg` is not stated here. Appendix I
  points to the **low byte** (sections 5.9 and 5.14). ⚠ Confirm on the real machine.
- `real` is **IEEE single precision**, 2 words.
- `NIL` is `-1024` = `FC00` hex, as `LDCN` pushes.
- `semp` is used in `tib.hangp` but not declared on these pages. Appendix I declares it:
  `semp = ^semtrix`, a 2-word record, `count` then `waitq` (sections 5.9, 5.12).

---

## 2. The operators (Table C-3)

Columns as printed: mnemonic, opcode in hex, parameters, semantics. Parameter types (from Table
C-1): **UB** unsigned byte; **SB** signed byte; **DB** "don't care" byte (0..127); **B** "big":
one byte if 0..127, else two bytes with bit 7 of the first cleared and used as the high byte;
**W** a word, low byte first.

### 2.1 Constant one-word loads (C-35)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| SLDCi | 00..1F | | Short Load Word Constant. `sp := sp - 1; sp^.i := i{0..31}` |
| LDCN | 98 | | Load Constant Nil. `sp := sp - 1; sp^.p := NIL` |
| LDCB | 80 | UB | Load Constant Byte. `sp := sp - 1; sp^.i := UB` |
| LDCI | 81 | W | Load Constant Word. `sp := sp - 1; sp^.i := W` |
| LCA | 82 | B | Load Constant Address. `sp := sp - 1; sp^.p := segb + B` |

### 2.2 Local one-word loads and store (C-35)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| SLDLi | 20..2F | | Short Load Local Word. `sp := sp - 1; sp^.i := (mp + mscw_sz - 1 + i{1..16})^.i` |
| LDL | 87 | B | Load Local Word. `sp := sp - 1; sp^.i := (mp + mscw_sz - 1 + B)^.i` |
| LLA | 84 | B | Load Local Address. `sp := sp - 1; sp^.p := mp + mscw_sz - 1 + B` |
| STL | A4 | B | Store Local Word. `(mp + mscw_sz - 1 + B)^.i := sp^.i; sp := sp + 1` |

### 2.3 Global one-word loads and store (C-35)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| SLDOi | 30..3F | | Short Load Global Word. `sp := sp - 1; sp^.i := (bp + mscw_sz - 1 + i{1..16})^.i` |
| LDO | 85 | B | Load Global Word. `sp := sp - 1; sp^.i := (bp + mscw_sz - 1 + B)^.i` |
| LAO | 86 | B | Load Global Address. `sp := sp - 1; sp^.p := bp + mscw_sz - 1 + B` |
| SRO | A5 | B | Store Global Word. `(bp + mscw_sz - 1 + B)^.i := sp^.i; sp := sp + 1` |

Variable offset 1 is the word just above the mark stack control word: `mp + mscw_sz - 1 + 1`.

### 2.4 Intermediate one-word loads and store (C-36)

```pascal
LOD  89  DB,B  { Load Intermediate Word. }
    lm := mp;
    for i := 1 to DB do lm := lm^.m.msstat;
    sp := sp - 1; sp^.i := (lm + mscw_sz - 1 + B)^.i

LDA  88  DB,B  { Load Intermediate Address. }
    lm := mp;
    for i := 1 to DB do lm := lm^.m.msstat;
    sp := sp - 1; sp^.p := lm + mscw_sz - 1 + B

STR  A6  DB,B  { Store Intermediate Word. }
    lm := mp;
    for i := 1 do DB do lm := lm^.m.msstat;      { sic: "1 do DB" -- read "1 to DB" }
    (lm + mscw_sz - 1 + B)^.i := sp^.i; sp := sp + 1
```

### 2.5 Indirect one-word store (C-36)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| STO | C4 | | Store Indirect. `(sp + 1)^.p^.i := sp^.i; sp := sp + 2` |

### 2.6 Extended one-word loads and store (C-36)

Two helpers are defined on this page and used by later operators:

```pascal
procedure Raise(err: integer);
    sp := sp - 1; sp^.i := err;
    CXG 2,2;

{ All references to ssv are through getsegb }
function getsegb(segno: integer): memp;
    if segno < 128 then
        getsegb := ssv^.sv[segno]^.segbase
    else
        getsegb := ctp^.t.sibs[segno - 128]^.segbase;
```

```pascal
LDE  9A  UB,B  { Load Word Extended. }
    sp := sp - 1;
    sp^.i := (ssv^.sv[UB]^.segbase + B)^.i

LAE  9B  UB,B  { Load Address Extended. }
    sp := sp - 1; sp^.p := ssv^.sv[UB]^.segbase + B

STE  D9  UB,B  { Store Word Extended. }
    (ssv^.sv[UB]^.segbase + B)^.i := sp^.i; sp := sp + 1
```

⚠ `LDE`, `LAE` and `STE` are printed with `ssv^.sv[UB]` directly, although the comment just
above says every reference to `ssv` goes through `getsegb`, and `UB` can be 128..255. Read them
as `getsegb(UB)` unless the real machine shows otherwise.

`Raise` pushes the error number and calls `CXG 2,2`: segment 2, procedure 2 (the system's
execution-error handler). `CXG` is defined with the procedure calls (section 2.19).

### 2.7 Multiple-word loads and stores — sets and reals (C-37)

```pascal
LDC  83  B,UB  { Load Multiple Word Constant. }
    src := segb + B + UB;
    for i := 1 to UB do (sp - i)^.i := (src - i)^.i;
    sp := sp - UB

LDM  D0  UB    { Load Multiple Words. }
    src := sp^.p + UB; sp := sp + 1;
    for i := 1 to UB do (sp - i)^.i := (src - i)^.i;
    sp := sp - UB

STM  8E  UB    { Store Multiple Words. }
    dst := (sp^.p + UB)^.p;                         { ⚠ as printed; see note }
    for i := 0 to UB - 1 do (dst + i)^.i := (sp + i)^.i;
    sp := sp + UB + 1
```

⚠ `STM`: the first line is printed `dst := (sp^.p + UB)^.p` (checked at 400 dpi). Table C-1
says the block of `UB` words is on top of the stack and the destination pointer is under it,
and the loop copies from `sp + 0 .. sp + UB - 1`, so the pointer is the word at `sp + UB`:
`dst := (sp + UB)^.p`. The printed form would dereference the first data word as a pointer.
Implement `(sp + UB)^.p`. Settled by real code: section 7.5.

### 2.8 Byte arrays (C-37)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| LDB | A7 | | Load Byte. `(sp + 1)^.i := (sp + 1)^.b[sp^.i]; sp := sp + 1` |
| STB | C8 | | Store Byte. `(sp + 2)^.b[(sp + 1)^.i] := sp^.i; sp := sp + 3` |

`b[]` indexes bytes from the word address: byte *n* of a word array starting at word *w*.
Which half of a word is byte 0 follows the Pascal-100's byte sex, low byte at the even byte
address (Pascal-100 manual, IEEE byte order).

### 2.9 Record and array indexing and assignment (C-38)

```pascal
MOV   C5  B        { Move Words. }
    src := sp^.p; dst := (sp + 1)^.p; sp := sp + 2;
    for i := 0 to B - 1 do
        (dst + i)^.i := (src + i)^.i

SINDi 78..7F       { Short Index and Load Word. }
    sp^.i := (sp^.p + i{0..7})^.i

IND   E6  B        { Index and Load Word. }
    sp^.i := (sp^.p + B)^.i

INC   E7  B        { Increment Field Pointer. }
    sp^.p := sp^.p + B

IXA   D7  B        { Index Array. }
    (sp + 1)^.p := (sp + 1)^.p + sp^.i * B;
    sp := sp + 1

IXP   D8  UB1,UB2  { Index Packed Array. }
    var inx: integer;
    inx := sp^.i;
    (sp + 1)^.p := (sp + 1)^.p + inx div UB1;
    sp^.i := UB2; sp := sp - 1;
    sp^.i := (inx mod UB1)*UB2

LDP   C9           { Load A Packed Field. }
    (sp + 2)^.i := (sp + 2)^.i<sp^.i : (sp + 1)^.i>;   { ⚠ as printed; see note }
    sp := sp + 2

STP   CA           { Store into a packed field. }
    (sp + 3)^.p^.i<(sp + 1)^.i : (sp + 2)^.i> := sp^.i;
    sp := sp + 4
```

A packed-field reference on the stack is three words, as `IXP` leaves it: the **start bit** on
top, the **field width** under it, the **word pointer** under that (`i<x:y>` = start bit *x*,
*y* bits).

⚠ `LDP` is printed `(sp + 2)^.i<…>` (checked at 400 dpi): it would take the field out of the
pointer word itself. `STP`, on the same page, dereferences: `(sp + 3)^.p^.i<…>`. Read `LDP` as
`(sp + 2)^.i := (sp + 2)^.p^.i<sp^.i : (sp + 1)^.i>`. Corroborated by the IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983), p. 1-61:
"Replace the packed field pointer TOS with the field it designates ... right-justified and
zero-filled."

### 2.10 Logicals (C-38, C-39)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| LAND | A1 | | Logical AND. `(sp + 1)^.bool := (sp + 1)^.i and sp^.i; sp := sp + 1` |
| LOR | A0 | | Logical OR. `(sp + 1)^.bool := (sp + 1)^.i or sp^.i; sp := sp + 1` |
| LNOT | E5 | | Logical NOT. `sp^.i := not sp^.i` |
| BNOT | 9F | | Boolean NOT. `sp^.bool := (not sp^.i) and 1` ⚠ 9F is also `RBP`; see section 2.22 |
| LEUSW | B4 | | Compare Unsigned Word <=. `(sp + 1)^.bool := ((sp + 1)^.p <= sp^.p); sp := sp + 1` |
| GEUSW | B5 | | Compare Unsigned Word >=. `(sp + 1)^.bool := ((sp + 1)^.p >= sp^.p); sp := sp + 1` |

`LAND`, `LOR` and `LNOT` work on the whole 16-bit word (the operands are `.i`); `BNOT` keeps
only bit 0. Unsigned compares use `.p`, i.e. the operands as unsigned addresses.

### 2.11 Integers (C-39, C-40)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| ABI | E0 | | Absolute Value of Integer. `sp^.i := Abs (sp^.i)` |
| NGI | E1 | | Negate Integer. `sp^.i := -sp^.i` |
| DUP1 | E2 | | Copy Word. `sp := sp - 1; sp^.i := (sp + 1)^.i;` |
| ADI | A2 | | Add Integers. `(sp + 1)^.i := (sp + 1)^.i + sp^.i; sp := sp + 1` |
| SBI | A3 | | Subtract Integers. `(sp + 1)^.i := (sp + 1)^.i - sp^.i; sp := sp + 1` |
| MPI | 8C | | Multiply Integers. `(sp + 1)^.i := (sp + 1)^.i * sp^.i; sp := sp + 1` |
| DVI | 8D | | Divide Integers. `if sp^.i = 0 then Raise (div_by_zero_error); (sp + 1)^.i := (sp + 1)^.i div sp^.i; sp := sp + 1` |
| MODI | 8F | | Modulo Integers. `if sp^.i <= 0 then Raise (mod_by_nonpos_error); (sp + 1)^.i := (sp + 1)^.i mod sp^.i; sp := sp + 1` `{ -x mod x returns x not 0 }` |
| CHK | CB | | Check Against Subrange Bounds. `if ((sp + 1)^.i <= (sp + 2)^.i) and ((sp + 2)^.i <= sp^.i) then sp := sp + 2 else Raise (range_error)` |
| EQUI | B0 | | Compare Integer =. `(sp + 1)^.bool := ((sp + 1)^.i = sp^.i); sp := sp + 1` |
| NEQI | B1 | | Compare Integer <>. `(sp + 1)^.bool := ((sp + 1)^.i <> sp^.i); sp := sp + 1` |
| LEQI | B2 | | Compare Integer <=. `(sp + 1)^.bool := ((sp + 1)^.i <= sp^.i); sp := sp + 1` |
| GEQI | B3 | | Compare Integer >=. `(sp + 1)^.bool := ((sp + 1)^.i >= sp^.i); sp := sp + 1` |

- `CHK`: the stack holds the value, then the lower bound, then the upper bound on top. On
  success the bounds are popped and the value stays.
- ⚠ `MODI`: the printed comment says `-x mod x` returns `x`, not `0`. That is a statement about
  the hardware's result, not standard Pascal. Settled on the real machine: `-6 mod 3` is `3`
  (section 7.12).
- ⚠ The error numbers (`div_by_zero_error`, `mod_by_nonpos_error`, `range_error`, …) are named,
  not numbered, in Table C-3. Their values are in the runtime-error table (Pascal-100 manual
  Appendix H, B.1); to be tied together when the core is built. Settled from the OS's own message list: section 7.9.
- ⚠ Overflow of `ADI`/`SBI`/`MPI` is not mentioned: assume 16-bit two's-complement wrap.

### 2.12 Reals (C-40)

`{ Over/underflow causes floating-point exception to be raised. }`

| Op | Hex | Par | Semantics |
|---|---|---|---|
| FLT | CC | | Float Top-of-Stack. `(sp - real_sz + 1)^.r := Float (sp^.i); sp := sp - real_sz + 1` |
| TNC | BE | | Truncate Real. `(sp + real_sz - 1)^.i := Truncate (sp^.r); sp := sp + real_sz - 1` |
| RND | BF | | Round Real. `(sp + real_sz - 1)^.i := Round (sp^.r); sp := sp + real_sz - 1` |
| ABR | E3 | | Absolute Value of Real. `sp^.r := Abs (sp^.r)` |
| NGR | E4 | | Negate Real. `sp^.r := -sp^.r` |
| DUP2 | C6 | | Copy Doubleword. `sp := sp - 2; sp^.i := (sp + 2)^.i; (sp + 1)^.i := (sp + 3)^.i;` |

⚠ A real is two words (`real_sz`), IEEE single precision (C-34). Which of the two words holds
the sign and exponent is not stated here. To settle from a constant in compiled code or on the
real machine. Settled by real code: section 7.4.

### 2.13 Reals, continued (C-41)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| ADR | C0 | | Add Reals. `(sp + real_sz)^.r := (sp + real_sz)^.r + sp^.r; sp := sp + real_sz` |
| SBR | C1 | | Subtract Reals. `(sp + real_sz)^.r := (sp + real_sz)^.r - sp^.r; sp := sp + real_sz` |
| MPR | C2 | | Multiply Reals. `(sp + real_sz)^.r := (sp + real_sz)^.r * sp^.r; sp := sp + real_sz` |
| DVR | C3 | | Divide Reals. `(sp + real_sz)^.r := (sp + real_sz)^.r / sp^.r; sp := sp + real_sz` |
| EQUREAL | CD | | Compare Real =. `(sp + 2*real_sz - 1)^.bool := ((sp + real_sz)^.r = sp^.r); sp := sp + 2*real_sz - 1` |
| LEQREAL | CE | | Compare Real <=. `(sp + 2*real_sz - 1)^.bool := ((sp + real_sz)^.r <= sp^.r); sp := sp + 2*real_sz - 1` |
| GEQREAL | CF | | Compare Real >. `(sp + 2*real_sz - 1)^.bool := ((sp + real_sz)^.r >= sp^.r); sp := sp + 2*real_sz - 1` |

⚠ `GEQREAL` is titled "Compare Real >." but its body (and its name) is `>=`. Implement `>=`.

### 2.14 Sets (C-42, C-43)

A set on the stack is its **length in words on top** (`sp^.i`), with the set words above it:
word 0 at `sp + 1`. Bit *n* of the set is `sb[n]`, counting from bit 0 of word 0.

```pascal
ADJ  C7  UB  { Adjust Set. }
    var len: integer;
    len := sp^.i;
    src := sp + 1; dst := sp + len - UB + 1;
    if len > UB then { shrink set }
        for i := 1 to UB do
            (dst + UB - i).i := (src + UB - i).i      { sic: no "^" }
    else if len < UB then { expand set }
        for i := 0 to len - 1 do
            (dst + i)^.i := (src + i)^.i;
        for i := len to UB - 1 do
            (dst + i)^.i := 0;
    sp := sp + len - UB + 1

SRS  BC      { Build Subrange Set. }
    var hi,lo,len: integer;
    hi := sp^.i; lo := (sp + 1)^.i;
    if (0 <= hi) and (hi <= bset_sz-1) and
       (0 <= lo) and (lo <= bset_sz-1)
    then
        if lo > hi then
            sp := sp + 1; sp^.i := 0 {Null set}
        else
            len := hi div word_sz + 1;
            sp := sp - len + 1; sp^.i := len;
            for i := 0 to len * word_sz - 1 do
                (sp + 1)^.sb[i] := (lo <= i) and (i <= hi);
    else
        Raise(range_error)

INN  DA      { Set Membership. }
    var len, val: integer;
    len := sp^.i; val := (sp + len + 1)^.i;
    if (0 <= val) and (val <= len * word_sz - 1) then
        (sp + len + 1)^.bool := (sp + 1)^.sb[val]
    else (sp + len + 1)^.bool := false;
    sp := sp + len + 1

UNI  DB      { Set Union. }
    var len0,len1: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i;
    if len1 >= len0 then {best case move & cut back}
        src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
        for i := 0 to len0 - 1 do
            (dst + i)^.i := (dst + i)^.i or (src + i)^.i;
        sp := sp + len0 + 1;
    else
        src := (sp + len0 + 2)^.p; dst := (sp + 1)^.p;
        for i := 0 to len1 - 1 do
            (dst + i)^.i := (dst + i)^.i or (src + i)^.i;
        { Move set up }
        src := lp + len0; dst := sp + len0 + len1 + 1;   { sic: "lp" }
        for i := 0 to len0 do { move length word }
            (dst - i)^.i := (src - i)^.i;
        sp := sp + len1 + 1

INT  DC      { Set Intersection. }
    var len0,len1: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i;
    if len0 = 0 then
        sp := sp + len1 + 1; sp^.i := 0
    else if len1 = 0 then
        sp := sp + len0 + 1
    else if len1 > len0 then {best case move & cut back}
        src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
        for i := 0 to len0 - 1 do
            (dst + i)^.i := (dst + i)^.i and (src + i)^.i;
        for i := len0 to len1 - 1 do
            (dst + i)^.i := 0;
        sp := sp + len0 + 1;
    else
        dst := (sp + len0 + 2)^.p; src := (sp + 1)^.p;
        for i := 0 to len1 - 1 do
            (dst + i)^.i := (dst + i)^.i and (src + i)^.i;
        sp := sp + len0 + 1
```

⚠ Notation slips on these pages, with the reading the stack layout requires:

- `ADJ` shrink: `(dst + UB - i).i` lacks the `^`; read `(dst + UB - i)^.i := (src + UB - i)^.i`.
- `UNI`, `INT`: the operands are sets **held on the stack**, not pointers to sets. So
  `src := (sp + 1)^.p` and `dst := (sp + len0 + 2)^.p` must mean the **addresses** `sp + 1` and
  `sp + len0 + 2` (the first word of each set), as `ADJ` writes them (`src := sp + 1`).
- `UNI`: `src := lp + len0` uses `lp`, which is declared nowhere; read `sp + len0` (the length
  word of the result is moved up with the set).
- `INT`, last branch (`len1 <= len0`): the result stays in the deeper set's place with its own
  length `len1`; the top set's words `len1 .. len0 - 1` are dropped, which is correct for an
  intersection (they meet only zero words). Not a slip; noted so it is not "fixed".

### 2.15 Sets, continued (C-44, C-45)

```pascal
DIF     DD  { Set Difference. }
    var len0,len1: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i;
    if len0 = 0 then
        sp := sp + 1
    else if len1 = 0 then
        sp := sp + len0 + 1
    else if len1 > len0 then {best case move & cut back}
        src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
        for i := 0 to len0 - 1 do
            (dst + i)^.i := (dst + i)^.i and not (src + i)^.i;
        sp := sp + len0 + 1;
    else
        dst := (sp + len0 + 2)^.p; src := (sp + 1)^.p;
        for i := 0 to len1 - 1 do
            (dst + i)^.i := (dst + i)^.i and not (src + i)^.i;
        sp := sp + len0 + 1

EQUPWR  B6  { Set Compare =. }
    var len0,len1,minl,maxl: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i; i := 0;
    minl := min(len0,len1); maxl := max(len0,len1);
    src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
    while (i < minl) and ((src + i)^.p = (dst + i)^.p) do
        i := i + 1;
    if len0 > len1 then
        while (i < maxl) and ((src + i)^.p = 0) do i := i + 1
    else if len1 > len0 then
        while (i < maxl) and ((dst + i)^.p = 0) do i := i + 1;
    sp := sp + len0 + len1 + 1; sp^.bool := (i >= maxl)

LEQPWR  B7  { Set Compare <= (Subset of). }
    var len0,len1,minl,maxl: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i; i := 0;
    minl := min(len0,len1); maxl := max(len0,len1);
    src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
    while (i < minl) and
          ((src + i)^.p = (dst + i)^.p or (src + i)^.p) do
        i := i + 1;
    if len1 > len0 then
        while (i < maxl) and ((dst + i)^.p = 0) do i := i + 1;
    else i := maxl;
    sp := sp + len0 + len1 + 1; sp^.bool := (i >= maxl)

GEQPWR  B8  { Set Compare >= (Superset of). }
    var len0,len1,minl,maxl: integer;
    len0 := sp^.i; len1 := (sp + len0 + 1)^.i; i := 0;
    minl := min(len0,len1); maxl := max(len0,len1);
    src := (sp + 1)^.p; dst := (sp + len0 + 2)^.p;
    while (i < minl) and
          ((dst + i)^.p = (dst + i)^.p or (src + i)^.p) do
        i := i + 1;
    if len1 < len0 then
        while (i < maxl) and ((src + i)^.p = 0) do i := i + 1;
    else i := maxl;
    sp := sp + len0 + len1 + 1; sp^.bool := (i >= maxl)
```

The `(sp + k)^.p` slip of section 2.14 applies to `DIF` and the three compares as well: read the
address `sp + k`. In the compares, `src` is the set on top (the right operand) and `dst` the one
under it (the left operand): `LEQPWR` tests left ⊆ right (each left word equals itself OR'd
with the right word), and `GEQPWR` tests left ⊇ right.

⚠ `LEQPWR`/`GEQPWR`: as printed, `else i := maxl` runs whenever the left set is not the longer
one (`LEQPWR`) or the right set is not the longer one (`GEQPWR`), **even if the first loop
stopped early on a word that failed the test**. That makes `{1} <= {2}` (equal lengths) true.
The intent must be: the result is true only if the first loop reached `minl`, and then the
longer set's extra words are checked on the side that matters (`LEQPWR`: the left set's extra
words must be 0; extra words of the right set cannot spoil a subset). Implement that. the IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983), p. 1-69,
agrees on the meaning: `LEPWR` "push true if TOS-1 is a subset of TOS", `GEPWR` the superset
test, and `DIF` is "TOS-1 AND NOT TOS".

### 2.16 Byte-array compares (C-45)

```pascal
EQUBYT  B9  B  { Equal Byte Array Compare. }
    src := sp^.p; dst := (sp + 1)^.p; i := 0;
    while (i < B) and (src^.b[i] = dst^.b[i]) do
        i := i + 1;
    sp := sp + 1; sp^.bool := (i >= B)

LEQBYT  BA  B  { Less or Equal Byte Array Compare. }
    src := sp^.p; dst := (sp + 1)^.p; i := 0;
    while (i < B) and (src^.b[i] <= dst^.b[i]) do
        i := i + 1;
    sp := sp + 1; sp^.bool := (i >= B)

GEQBYT  BB  B  { Greater or Equal Byte Array Compare. }
    src := sp^.p; dst := (sp + 1)^.p; i := 0;
    while (i < B) and (src^.b[i] >= dst^.b[i])          { sic: no "do" }
        i := i + 1;
    sp := sp + 1; sp^.bool := (i >= B)
```

⚠ As printed, `LEQBYT` and `GEQBYT` are **element-wise**, not lexicographic: "every byte of
`src` <= the matching byte of `dst`". Pascal compares packed character arrays
lexicographically (first differing byte decides). The printed operand order also looks
reversed: `src` is the top-of-stack (right) operand, so `LEQBYT` as printed tests right <= left
byte by byte. Both are likely simplifications in the description. Settled on the real machine:
the compare is lexicographic, TOS-1 against TOS (section 7.12). Corroboration:
- the IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983), p. 1-70: "Push the Boolean result of the byte array comparison TOS-1 <= TOS".
  So the left operand is TOS-1 (`dst`) and the right is TOS (`src`): the printed C-3 has the
  operands reversed.
- The UCSD II.0 *Users Manual* (Mar 1979) says that `<=`, `<`, `>=` and `>` byte-array
  compares "are only emitted for packed arrays of char", i.e. for Pascal string comparison,
  which is lexicographic.

Implement: compare `dst` (TOS-1) with `src` (TOS) lexicographically, the first differing
byte deciding. Still check one case (e.g. `'AZ' <= 'BA'`, true) on the real machine.

### 2.17 Jumps (C-45, C-46)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| UJP | 8A | SB | Unconditional Jump. `pc := pc + SB` |
| FJP | D4 | SB | False Jump. `if not sp^.bool then pc := pc + SB; sp := sp + 1` |
| EFJ | D2 | SB | Equal False Jump. `if (sp + 1)^.i <> sp^.i then pc := pc + SB; sp := sp + 2` |
| NFJ | D3 | SB | Not Equal False Jump. `if (sp + 1)^.i = sp^.i then pc := pc + SB; sp := sp + 2` |
| UJPL | 8B | W | Unconditional Long Jump. `pc := pc + W` |
| FJPL | D5 | W | False Long Jump. `if not sp^.bool then pc := pc + W; sp := sp + 1` |

```pascal
XJP  D6  B  { Case Jump. }
    if ((segb + B)^.i <= sp^.i) and
       ((segb + B + 1)^.i >= sp^.i) then
        pc := pc + (segb + B + 2 + sp^.i - (segb + B)^.i)^.p;
    sp := sp + 1
```

- `XJP`: the case table is in the segment's constant area at word `segb + B`: the low bound,
  the high bound, then one word per case value holding a **pc-relative offset**. Out of range
  falls through to the next instruction.
- ⚠ The table does not say where `pc` points when the offset is added (after the opcode, or
  after its parameters). Table C-1 or compiled code settles it; the usual reading is "after the
  whole instruction". Settled by real code: section 7.3.
- `pc` is a **byte** address (`msipc` is a "byte index in return code seg"). Whether it is
  relative to `segb` or absolute is not stated on these pages. Settled: a byte offset from `segb`, section 7.1.

### 2.18 Procedure and function calls (C-46)

```pascal
procedure createmscw;
    { data_sz = (segb + segb^.i - procno)^.i }
    sp := sp - mscw_sz - data_sz;
    if (sp < splow) or (data_sz + mscw_sz > sp - splow)
        then Raise(stack_overflow);
    lm := mp; mp := sp;
    mp^.m.msdynl := lm; mp^.m.msipc := pc

CPL  90  UB     { Call Local Procedure. }
    createmscw;
    mp^.m.msstat := lm; mp^.m.msseg := 0;
    pc := (segb + segb^.i - UB - 1)^.p

CPG  91  UB     { Call Global Procedure. }
    createmscw;
    mp^.m.msstat := bp; mp^.m.msseg := 0;
    pc := (segb + segb^.i - UB - 1)^.p

CPI  92  DB,UB  { Call Intermediate Procedure. }
    createmscw;
    mp^.m.msseg := 0;
    lm := mp;
    for i := 1 to DB do lm := lm^.m.msstat;
    mp^.m.msstat := lm;
    pc := (segb + segb^.i - UB - 1)^.p
```

- The **procedure dictionary** is found through the segment's first word: `segb^.i` is an
  offset from `segb`, and procedure *n*'s entry is the word at `segb + segb^.i - n - 1`
  (`createmscw` reads `data_sz` from `segb + segb^.i - procno`, one word further on). ⚠ Which
  word is the procedure's entry address and which its data size, and the units of each, to be
  confirmed against a real code segment from the user's disks. Settled by real code, and the
  layout differs from C-3: section 7.2.
- `createmscw` uses `procno` for the procedure being called (the `UB` of the caller).
- ⚠ `CPI` sets `lm := mp` **after** `createmscw`, so the static chain is walked from the **new**
  mark stack, whose `msstat` is not yet set. As printed this walks garbage. The likely intent is
  to walk from the caller's `mp` (the `lm` that `createmscw` leaves). the IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983), p. 1-73,
  supports that: the callee is "at lex level DB less than the currently executing procedure
  ... Use that activation record's static link". The exact count (`DB` or `DB - 1` links) is
  to be checked against compiled code. Settled: `DB` links from the caller's `mp`, section 7.6.

### 2.19 External and formal calls, return (C-47)

```pascal
CXL  93  UB1,UB2     { Call Local External Procedure. }
    createmscw;
    mp^.m.msstat := lm;
    mp^.m.msseg := (segb + segb^.i)^.b[0];
    segb := ssv^.sv[UB1]^.segbase;
    ssv^.sv[UB1]^.segrefs := ssv^.sv[UB1]^.segrefs + 1;
    pc := (segb + segb^.i - UB - 1)^.p

CXG  94  UB1,UB2     { Call Global External Procedure. }
    createmscw;
    mp^.m.msstat := bp;
    mp^.m.msseg := (segb + segb^.i)^.b[0];
    segb := ssv^.sv[UB1]^.segbase;
    ssv^.sv[UB1]^.segrefs := ssv^.sv[UB1]^.segrefs + 1;
    pc := (segb + segb^.i - UB - 1)^.p

CXI  95  UB1,DB,UB2  { Call Intermediate External Procedure. }
    createmscw;
    lm := mp;
    for i := 1 to DB do lm := lm^.m.msstat;
    mp^.m.msstat := lm;
    mp^.msseg := (segb + segb^.i)^.b[0];                  { sic: "mp^.msseg" }
    segb := ssv^.sv[UB1]^.segbase;
    ssv^.sv[UB1]^.segrefs := ssv^.sv[UB1]^.segrefs + 1;
    pc := (segb + segb^.i - UB - 1)^.p

CPF  97              { Call Formal Procedure. }
    var ls: memtrix;
    ls := sp^.i; lm := (sp + 1)^.p; sp := sp + 2;
    createmscw;
    mp^.m.msseg := ls.b[1];
    segb := ssv^.sv[ls.b[1]]^.segbase;
    ssv^.sv[ls.b[1]]^.segrefs :=
        ssv^.sv[ls.b[1]]^.segrefs + 1;
    mp^.m.msstat := lm;
    pc := (segb + segb^.i - ls.b[0] - 1)^.p

RPU  96  B           { Return From User Procedure. }
    sp := mp; lm := mp;
    mp := lm^.m.msdynl; pc := lm^.m.msipc;
    if lm^.m.msseg <> 0 then
        segb := ssv^.sv[lm^.m.msseg].segbase;             { sic: no "^" before .segbase }
        ssv^.sv[m.msseg]^.segrefs :=                      { sic: "m.msseg" }
            ssv^.sv[m.msseg]^.segrefs - 1;
    sp := sp + B + mscw_sz
```

- The **segment dictionary word** is the word at `segb + segb^.i`: its byte 0 is this
  segment's number (saved in the caller's mark stack as `msseg`), and the procedure entries
  lie below it (`segb + segb^.i - n - 1` for procedure *n*).
- `msseg = 0` means "same segment": local calls store 0, and `RPU` only reloads `segb` when
  `msseg <> 0`. ⚠ So segment 0 itself can never be returned to by `RPU`; presumably segment 0
  never calls out with `CXL`/`CXG`/`CXI`. *Settled in 7.10: segment 0 calls out once and is
  never returned to.*
- `RPU`: `B` is the number of words above the mark stack to discard (the procedure's data,
  leaving a function result).
- `CPF`: a formal (procedure-parameter) value is two words on the stack: on top a word whose
  byte 0 is the procedure number and byte 1 the segment number, under it the static link.

⚠ Slips and open points on this page:

- `CXL`, `CXG`, `CXI` take the procedure number from `UB` in the last line, but their parameters
  are `UB1,UB2` (and `UB1,DB,UB2`): read `UB2`.
- They index `ssv^.sv[UB1]` directly, not through `getsegb` (as `LDE`, section 2.6).
- `CXI` walks the static chain from `mp` **after** `createmscw`, the same problem as `CPI`
  (section 2.18).
- `RPU` decrements `segrefs` of `m.msseg`. `m` is undeclared. The caller's segment (`msseg`) is
  the segment being **returned to**, but the call incremented the **callee's** `segrefs`
  (`UB1`). Read: decrement the segment being left (the current `segb`'s number) before
  reloading `segb`. Confirm on the real machine.
- `CPF` stores the **callee's** segment number (`ls.b[1]`) in `msseg`, where `CXL` stores the
  **caller's**. As printed, `RPU` would then "return" into the callee's segment. Read `CPF` as
  storing the caller's segment number, as `CXL` does. Confirm on the real machine.

### 2.20 Static link (C-48)

| Op | Hex | Par | Semantics |
|---|---|---|---|
| LSL | 99 | DB | Load Static Link Onto Stack. `lm := mp; for i := 1 to DB do lm := lm^.m.msstat; sp := sp - 1; sp^.p := lm` |

### 2.21 Concurrency control (C-48, C-49)

```pascal
var qhead, qtask: tibp;

procedure updatetib;
    ctp^.t.mp := mp; ctp^.t.bp := bp;
    ctp^.t.sp := sp; ctp^.t.ipc := pc;
    ctp^.t.segb := segb;

procedure enque;
    var t1,t2: tibp;
    t1 := qhead; t2 := NIL;
    while t1 <> NIL do
        if t1^.prior < qtask^.prior then goto 1;
        t2 := t1; t1 := t1^.qlink;
1:
    qtask.qlink := t1;                                    { sic: "qtask.qlink" }
    if t2 = NIL then qhead := qtask
        else t2^.qlink := qtask

procedure deque;
    qtask := qhead; qhead := qhead^.qlink;

5: { taskswitch }
    updatetib;
6:
    while rq = NIL do
        if an_interrupt then
            ctp := NIL;
            sp := sp - 1;
            sp^.i := int_vec_address; {hardware generated}
            sp^.p := int_vec_address^.p;
            goto SIGNAL;
    qhead := rq; deque;
    rq := qhead; ctp := qtask;
    sp := ctp^.t.sp; mp := ctp^.t.mp; bp := ctp^.t.bp;
    pc := ctp^.t.pc; segb := ctp^.t.segb;
    { Fall through here as well as for all other
      operators implies fetch next instruction }

SIGNAL  DE  { Signal semaphore. }
    if sp^.p^.sem.count = 0 then
        if sp^.p^.sem.waitq <> NIL then
            qhead := sp^.p^.sem.waitq; deque;
            sp^.p^.sem.waitq := qhead; qhead := rq;
            enque; rq := qhead;
            if ctp = nil then goto 6;
            if ctp^.prior < qtask^.prior then
                qtask := ctp; qhead := rq; enque;
                rq := qhead;
                goto 5;
            else goto 3;
    sp^.p^.sem.count := sp^.p^.sem.count + 1
    if ctp = NIL then goto 6;
3:
    sp := sp + 1

WAIT    DF  { Wait on Semaphore. }
    if sp^.p^.sem.count = 0 then
        qhead := sp^.p^.sem.waitq;
        qtask := ctp; enque;
        sp^.p^.sem.waitq := qhead;
        goto 5;
    else sp^.p^.sem.count := sp^.p^.sem.count + 1;     { ⚠ as printed: read "- 1" }
    sp := sp + 1
```

- The ready queue `rq` and each semaphore's `waitq` are lists of TIBs linked through the TIB's
  first word, **in priority order** (highest first; `enque` inserts before the first task of
  lower priority). The code calls that link `qlink`; the TIB declaration (C-34) calls it
  `waitq`. Same word.
- **Interrupts** reach the P-machine only at label 6, when no task is ready (`rq = NIL`): the
  hardware supplies `int_vec_address`, the P-machine pushes the semaphore pointer stored there,
  and SIGNALs it with `ctp = NIL`. On the Pascal-100 that vector is word `FC60` (byte `1F8C0`);
  see `Digicomp Research Pascal-100.md` §5.3.
- A task switch (label 5) saves the running task's registers in its TIB (`updatetib`), then
  label 6 takes the head of `rq` and loads its registers.

⚠ Slips and open points:

- `WAIT` is printed `count := count + 1` when it does not block (checked at 400 dpi). A wait
  **decrements** a semaphore; read `count - 1`.
- `WAIT` (blocking) and `SIGNAL` (pre-empting) `goto 5` **without** `sp := sp + 1`, so the
  semaphore pointer stays on the suspended task's stack and is saved in its TIB. When that task
  resumes, the pointer would still be there. Either the operators pop it before the switch, or
  the resumed task's next instruction is expected to; confirm against compiled code (what
  follows a `WAIT` in a `PROCESS`). Settled: both pop it, section 7.7.
- Label 6 loads `pc := ctp^.t.pc`, but the TIB field is `ipc`. Same field.
- The interrupt path writes `sp^.i := int_vec_address` and then overwrites the same word with
  `sp^.p := int_vec_address^.p`; only the second assignment matters.

### 2.22 Miscellaneous (C-50)

```pascal
LPR   9D  { Load Processor Register. }
    if sp^.i >= 0 then updatetib;
    sp^.i := case sp^.i of
                 -3: rq;
                 -2: ssv;
                 -1: ctp;
                 1..maxint: (ctp + sp^.i)^.i

SPR   D1  { Store Processor Register. }
    if sp^.i >= -1 then updatetib;
    sp^.p^.i := case sp^.i of
                    -3: rq;
                    -2: ssv;
                    -1: ctp; goto 5; { Taskswitch }
                    1..maxint: (ctp + (sp + 1)^.i)^.i
    if sp^.i >= -1 then updatetib;
    sp := sp + 2

BPT   9E  { Break Point. }
    Raise(Breakpoint);

NOP   9C  { No Operation. }

SWAP  BD  { Swap Word. }
    i := sp^.i;
    sp^.i := (sp + 1)^.i;
    (sp + 1)^.i := i
```

- Register numbers (as the Pascal-100 manual, appendix F 3.7.3, lists them): -3 ready queue
  (`rq`, "RQP"), -2 segment vector (`ssv`, "SDP"), -1 current task (`ctp`), and a positive
  number *n* is word *n* of the current TIB (2 `splow`, 3 `spupr`, 4 `sp`, 5 `mp`, 6 `bp`,
  7 `ipc`, 8 `segb`). `LPR` saves the live registers into the TIB first, so reading a TIB
  register returns the current value.
- ⚠ `SPR` is garbled as printed: it assigns **to** `sp^.p^.i` **from** the registers, which is
  `LPR`'s direction, and the `1..maxint` case uses `(sp + 1)^.i` as the register number while
  the `case` selects on `sp^.i`. The Pascal-100 manual's opcode list (appendix H, B.5) states the
  operation: "Tos-1 is a register number (same definition as LPR). Store Tos in this
  register." The IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983), p. 1-78, says the same. So: register number `(sp + 1)^.i`, value `sp^.i`; for 1..maxint
  `(ctp + (sp + 1)^.i)^.i := sp^.i`, then reload the live registers from the TIB; storing
  `ctp` (-1) switches task (`goto 5`). Read the `case` as selecting on `(sp + 1)^.i`. Settled by real code: section 7.8.
- **`RBP`** (Return From Breakpoint) is **not in Table C-3**. Table C-1 (C-13) gives it:
  opcode 159 (`9F`), parameter `B`, "This acts like an RPU operator. The stack pointer is
  decremented by B." The Pascal-100 manual (appendix H, B.5) says the same.
- ⚠ **Opcode `9F` (159) is given to two operators**, and the sources split evenly:
  - `BNOT` = 9F: Table C-3 (C-39), Table C-1 (C-6), and the WD execution-time note
    (Technical Note M0015) reprinted in the Pascal-100 manual's appendix H.
  - `RBP` = 159: Table C-1 (C-13) and the Pascal-100 manual's opcode list (appendix H, B.5),
    which has no `BNOT`.

  - The IV.0 p-machine, which took over the III.0 extensions, has `BNOT` = 159 ("Complement
    the low order bit and clear the remainder of TOS") and **no `RBP` at all** (the IV.0 *Internal Architecture* (TI 2232400-0001, Apr 1983),
    pp. 1-63 and A-1).

  - The III.0 OS itself (release H3) declares `bnot = 159` among the operators it uses
    (Appendix I, section 5.1).

  So the later documents all say `BNOT`; only the 1979-81 opcode list says `RBP`. A likely
  story is that early III.0 microcode had `RBP` at `9F` and a later one replaced it with
  `BNOT`. Which one is in the user's chip is still unknown. One chip cannot decode both. **Settle on the real machine**: compile `b := not b` for a
  boolean `b` and look at the code the compiler makes, and run a `PMACHINE` that executes `9F`.

---

## 3. Differences from the Pascal-100 manual, appendix F

The Digicomp *Pascal-100 User's Manual* (March 1981) reprints an earlier WD description of the
same P-machine (appendix F, 3.7.3). Where the two disagree:

| Item | Pascal-100 appendix F (1979-81) | This manual, C-33/C-34 (1982) |
|---|---|---|
| TIB word 10 | `XXX: INTEGER; { NOT USED }` | `iorslt: integer; { Result of last I/O call. }` |
| Segment vector | `SEGVEC = ARRAY[0..15] OF SIBP` | `sibvec = array [0..127] of sibp`, plus `tib.sibs` for segments 128..255 |
| SIB | 5 fields, ending at `SEGUNIT` | adds `prevsp: memp { SP saved by getseg for relseg }` |
| TIB tail | `REGS` packed record, then `MAINTASK: BOOLEAN; STARTMSCW: MSCWP` | the register words only, no `MAINTASK`/`STARTMSCW` |
| Queues | "implemented by means of LIFO queues" | `enque` inserts in **priority order** (C-48) |
| Register -2 | "Segment Vector Pointer [SDP]" | `ssv`, "system segment vector" |
| Opcode `9F` | `RBP` in the opcode list (appendix H, B.5) | `BNOT` (C-3); see section 2.22 |

The TIB word offsets are the same in both, so register numbers 2-8 agree. The larger segment
vector and `iorslt` belong to the later operating system. Which one the user's disks expect is
settled at boot: the segment numbers that the user's `SYSTEM.PASCAL` passes to `CXL`/`CXG`.

---

## 4. Appendix H: boot and initialization diagnostic messages (H-1 to H-4)

Printed page H-*n* is PDF page *n* + 436.

Most of this appendix is about **WD's own machines**, not the Pascal-100: the SB1600 and
ME1600 boot PROMs. The Pascal-100 has no such PROM; its Z80 boots from the Disk 1's EPROM
(Digicomp manual §6.3). Those PROM sections are summarized only. The **operating system's
initialization numbers** (end of H.2) come from the III.0 OS itself and do apply.

The four boot actions (H-1): the PROM (or microcode, with no PROM) picks the boot disk and
loads the **primary bootstrap**; the primary boot loads the **secondary bootstrap**; the
secondary boot loads the parts of the operating system needed to start; the OS initializes.

### 4.1 SB1600 PROM (H.1) — WD machines only

Normal display `TESTING 1.23`. Built-in test (H.1.1): DRB (four diagnostic result bits) = `0000`
at Master Reset, `1111` at the first PROM instruction; fills memory with `0000`; **writes a TIB
to `7E00`**; tests high memory `8000`–`FBFF` (`T`), relocates the stack to high memory (`E`),
transfers to high memory (`S`), tests low memory `0000`–`7FFF` (`T`), memory/interrupt
timeouts (`I`), DMA registers (`N`), floppy registers (`G`), serial port B. Floppy boot (H.1.2):
drives 0–3 = units 4, 5, 9, 10; `.` per drive not ready, `1` on a ready drive, restore to track 0,
seek track 1 (switching density on failure), read track 1 (`2`), `3` if no read error, then
**task switch** to the software boot just read. Error codes (H.1.3): `a` high memory, `b` low
memory, `c` memory/interrupt reply timeout or interrupts wrongly enabled, `d` DMA registers,
`e` floppy registers, `f` port B, `h` floppy restore, `i` seek in either density, `j` track 1
read (CRC, lost data, record not found, DMA timeout).

Note for the P-machine: these addresses are **word** addresses (`FBFF` is the top of 64K words).
The PROM starts the software boot with a **task switch**, i.e. through a TIB, as Table C-3's
label 5/6 does.

### 4.2 ME1600 PROM and the secondary boot (H.2)

ME1600 PROM display `<f>`: `<` at start, `f` (floppy controller) or `w` (Winchester), `>` after a
good disk read. The primary boot then shows `floppy` or `winch` while it loads the secondary
boot; an incomplete word (`flo`) means no valid boot on the disk or a transfer problem. The
secondary boot then prints the **memory size** it assumes, a bootmake parameter. "Systems
called '128K-byte' actually have only 126K bytes of RAM that the system can use; the remaining
2K bytes are used for the PROM and hardware device communication."

Then the **OS initialization** writes numbers on top of one another. Where they stop:

| Stops at | Suggests |
|---|---|
| 6 | serial or parallel ports |
| 7 | floppy |
| 8 | winchester |
| 10 | serial ports |
| 11..14 | interrupts |
| 15, 16 | system configuration / unitallocation / winchester |
| 17 | clock / reading from system disk |
| 0..255 followed by `c` | problems clearing that unit |
| 21 | memory parity |
| 22..27 | system file initialization |
| 28 | system not loaded properly / disk reading problem |

After all numbers, the welcome message and then the prompt line at the top of the screen.
No numbers after the secondary boot's message means it failed to load the OS.

**Use for pascal100:** if a simulated boot stops, the last number on the screen names the
subsystem, as on the real machine. ⚠ Whether the Pascal-100's III.0 release prints the same
numbers is not stated (the Digicomp manual does not list them); compare with the user's real
boot screen.

---

## 5. Appendix I: III.0 operating system globals [H3] (I-1 to I-23)

Printed page I-*n* is PDF page *n* + 440.

"The following code represents the globals for release H3 of the III.0 Operating System. The
order of VAR declarations is stable and is guaranteed not to change from the declaration of
SYSCOM to the declaration for UNITABLE."

⚠ **Release.** This is release **H3** (1982). The user's Pascal-100 disks may carry an earlier
III.0 release (the Digicomp manual is dated March 1981). Where the user's `SYSTEM.PASCAL`
disagrees with this appendix, the disk wins. I-1 gives one marker: `oldmaxunit = 27`, "the old
(pre-H3) maximum unit number".

### 5.1 Constants (I-1, I-2)

```pascal
const
  { constants for pmachine statements }
  { Tib Registers }        { Execute errors }    { Operators }
  rqreg     = -3;          syserr = 0;           sto   = 196;   leusw = 180;
  ssvreg    = -2;          invinx = 1;           ldm   = 208;   geusw = 181;
  ctpreg    = -1;          noproc = 3;           ldb   = 167;   adi   = 162;
  priorreg  =  1;          stkovr = 4;           stb   = 200;   sbi   = 163;
  splowreg  =  2;          syioer = 9;           mov   = 197;   cxg   = 148;
  spuprreg  =  3;          uioerr = 10;          sind0 = 120;   cxi   = 149;
  spreg     =  4;          fperr  = 12;          sind1 = 121;   cpf   = 151;
  mpreg     =  5;          s2long = 13;          inc   = 231;   rpu   = 150;
  bpreg     =  6;                                bnot  = 159;   lsl   = 153;
  usvreg    = 11;                                ixa   = 215;   lpr   = 157;
                                                 bpt   = 158;   spr   = 209;

  { Definitions common to more than one set of types }
const
  vidleng      = 7;
  tidleng      = 15;
  maxdir       = 77;   { max number of entries in a directory }
  fblksize     = 512;  { standard disk block length }
  dirblk       = 2;    { disk addr of directory }
  agelimit     = 300;  { max age for gdirp...in ticks (5 seconds) }

  cmaxunit     = 255;
  oldmaxunit   = 27;   { the old (pre-H3) maximum unit number }
  maxsysunit   = 127;  { maximum number of system serial & parallel units }
  mindiskunit  = 4;

  configversion = 'WD02';

  { These are hardware determined numbers }
  maxdrive     = 3;    { 4 possible : 0..3 }
  { for the Winchester controller }
  maxheads     = 8;    { the maximum number of heads }
  maxcylinder  = 1023; { 1024 possible : 0..1023 }

type
  vid = string[vidleng];
  tid = string[tidleng];
  alpha = packed array [0..7] of char;
  window = packed array [0..0] of char;
  windowp = ^window;
  dp_integer = record
                 lo : integer;
                 hi : integer;
               end;
```

Notes:

- **Execute errors** are the numbers `Raise` passes (Table C-3 names them: `div_by_zero_error`,
  `range_error`, …). This list gives only those the OS uses by name: 0 system error, 1 invalid
  index, 3 no procedure, 4 stack overflow, 9 system I/O error, 10 user I/O error,
  12 floating-point error, 13 string too long. The full table is in the Pascal-100 manual,
  appendix H (B.1).
- **The OS itself uses `bnot = 159`** (`9F`). With section 2.22, this is the strongest evidence
  so far that the H3 microcode decodes `9F` as `BNOT`. It does not settle what the user's chip
  does.
- `usvreg = 11`: TIB word 11 is `sibs`, the pointer to the vector of SIBs for segments
  128..255 (C-34).
- `priorreg = 1`: TIB word 1 holds `prior` and `flags`.

### 5.2 Unit table types (I-3)

```pascal
{ Unitable related types -- Needs t_common }
unitnum   = 0..oldmaxunit;

devtype   = (invalid, floppydisk, parallel, serial, winchdisk {& others});
unitentry = packed record {an entry of unitable}
              uvid    : vid;      { VOLUME ID FOR UNIT }
              uisblkd : boolean;
              case utype : devtype of
                floppydisk, winchdisk : (ueovblk: integer);
                serial, parallel      : (portfor: integer)
            end { unitentry } ;

arrayunitable = array[0..cmaxunit] of ^unitentry;
```

### 5.3 Directory types (I-4)

```pascal
{ Directory related types }
                              { ARCHIVAL INFO...THE DATE }
daterec = packed record
            month: 0..12;   { 0 IMPLIES DATE NOT MEANINGFUL }
            day:   0..31;   { DAY OF MONTH }
            year:  0..100   { 100 IS TEMP DISK FLAG }
          end { DATEREC } ;

                              { DISK DIRECTORIES }
dirrange = 0..maxdir;

filekind = (untypedfile, xdskfile, codefile, textfile, infofile,
            datafile, graffile, fotofile, securedir);

direntry = record
             dfirstblk: integer; { FIRST PHYSICAL DISK ADDR }
             dlastblk:  integer; { POINTS AT BLOCK FOLLOWING }
             case dfkind: filekind of
               securedir,
               untypedfile: { ONLY IN DIR[0]...VOLUME INFO }
                 (dvid: vid;              { NAME OF DISK VOLUME }
                  deovblk: integer;       { LASTBLK OF VOLUME }
                  dnumfiles: dirrange;    { NUM FILES IN DIR }
                  dloadtime: integer;     { TIME OF LAST ACCESS }
                  dlastboot: daterec);    { MOST RECENT DATE SETTING }
               xdskfile,codefile,textfile,infofile,
               datafile,graffile,fotofile:
                 (dtid: tid;              { TITLE OF FILE }
                  dlastbyte: 1..fblksize; { NUM BYTES IN LAST BLOCK }
                  daccess: daterec)       { LAST MODIFICATION DATE }
           end { DIRENTRY } ;

dirp = ^directory;

directory = array [dirrange] of direntry;
```

The directory starts at block `dirblk` = 2 and holds up to 77 files (`maxdir`), the standard
UCSD layout. This is what a host tool needs to read files from the user's Pascal disk images.

### 5.4 Configuration record types (I-5)

```pascal
{ Configuration record related types }
{ declarations needed for the configuration table }
floppytype = (eight_inch, five_inch, {others}f2,f3,f4,f5,f6,f7,f8); {4 bits}
driverange = 0..maxdrive;
cylndrange = 0..maxcylinder;
diskunits  = mindiskunit..cmaxunit;
sysunits   = 1..maxsysunit;
sunitset   = set of sysunits;                 { 8 words}
unitset    = set of diskunits;                {16 words}

{ system configuration of unit number mapping to types of disk drives }

punitinfo = ^unitinfo;
unitinfo  = record
              cylinder : cylndrange;
              block    : integer;
              vollen   : integer;
            end;

configrec = record
              version    : packed array[0..3] of char;
                           { identifies this as a valid configrec }
                           { this field is initialized when loaded }

              drive      : packed array [diskunits] of driverange;
                           { the disk controller drive number of unit }

              { characteristics of a particular Winchester drive }
              winchdrive : array [driverange] of
                             record {this record unpacked for speed}
                               maxcyl         : cylndrange;
                               numofheads     : 0..maxheads;
                               blockspertrack : 0..255;
                               step_rate      : 0..15;
                             end;

              { map of unitnumbers to drive and disk location & length }
              pwinchunit : array [diskunits] of punitinfo;

            end; {configrec}
```

### 5.5 The configuration record on disk (I-6, I-7)

Printed twice, on facing pages, with small differences. I-6:

```pascal
static_configrec = { the form of the configuration record on disk }
  record
    version    : packed array[0..3] of char;      {2 words}
                 { identifies this as a valid configrec }

    serialset  : sunitset;                        {8 words}
                 { the system defined serial units }
    parallset  : sunitset;                        {8 words}
                 { the system defined parallel units }
    floppyset  : unitset;                         {16 words}
                 { those units that are on floppy drives }

    { this field is currently unused }
    floppydrive : packed array [0..7] of floppytype; {2 words}
                 { type of floppy drive }

    { remainder of the 64 words in the last sector on a floppy
      track, for added future data fields }
    reserved   : array [1..28] of integer;

    {----- fields below this line are only on Winchesters -----}

    winchset   : unitset;
                 { those units that are on Winchester drives }

    { map of unitnumbers to drive and disk location & length }
    winchunit  : array [diskunits] of unitinfo;

    { the form the configuration record will take in memory }
    dynamic_config : configrec;

  end; {static_configrec}
```

I-7 is the same record except: `version:` has no type, there are no word counts and no comment
on `reserved`, and `reserved : array [1..30] of integer`.

⚠ The two pages disagree on `reserved` (28 or 30 words). The I-6 word counts add up:
2 + 8 + 8 + 16 + 2 + 28 = **64 words**, "the 64 words in the last sector on a floppy track". So
I-6 is consistent with itself and I-7 looks like an older draft (with `reserved` 30, the record
would be 66 words). Read I-6.

### 5.6 The system communication area, SYSCOM (I-8, I-9)

```pascal
{ System communication area (syscom) and related types
  Needs definition of static_configrec, configrec, dirp.
  The needed definitions are in t_config, t_directry.
  (these types are used for pointers to these objects, so they could be
   replaced with integers, i.e.  type dirp = integer; ) }

{ declarations supporting idsearch / treesearch intrinsics --       }
{ compiler using idsearch will have set up rw table with correct    }
{ len for rwinfo, and have set syscom^.rwtable to point to it.      }
trsnodep = ^trsnode;                { symbol table node declaration }
trsnode  = record                   { -- used by treesearch }
             key   : alpha;
             rlink : trsnodep;
             llink : trsnodep;
           end;
idsinfo  = record                   { idsearch returns results via this }
             symcursor : 0..1023;   { "pseudo record". compiler must    }
             sy        : integer;   { declare vars in this order and    }
             op        : integer;   { pass its symcursor to idsearch.   }
             id        : alpha;
           end;
rwtblrec = record
             rwindex : array ['A'..'['] of integer;
             rwinfo  : array [0..0] of
                         record
                           id : alpha;
                           sy : integer;
                           op : integer;
                         end;
           end {rwtblrec};

                                    { SYSTEM COMMUNICATION AREA }
                                    { SEE INTERPRETERS...NOTE   }
                                    { THAT WE ASSUME BACKWARD   }
                                    { FIELD ALLOCATION IS DONE  }

syscomrec = record case integer of
              1 : ( boot_config : ^static_config
                                  { points to (temporary) location of
                                    the system configuration table at
                                    boot time, tables will be relocated
                                    later (in Initialize?) PLB });

              2 : ( config : ^configrec;
              unused : integer; { 1 spare word. }
              sysunit: integer;    { PHYSICAL UNIT OF BOOTLOAD }
              rwtable: ^rwtblrec;  { reserved word table for treesearch }
              gdirp: dirp;         { GLOBAL DIR POINTER,SEE VOLSEARCH }
              diskinfo: packed record
                          dseekrate: integer;  {STEP RATE FOR DISK DRIVE}
                          dreadrate: integer;  {DISK READ COMMAND}
                          dwriterate: integer; {DISK WRITE COMMAND}
                        end;
              auxinfo:  packed record
                          baudrates: packed array [0..7] of 0..15;
                                     { 2 words, indices [0,4] not used }
                          xonoff: packed array[0..7] of boolean;
                          clockvalue: integer;  { tick clock rate }
                          menudriven: boolean;  { using *system.menu }
                          transparent: packed array[0..7] of boolean;
                          { ignore special chars serial IO, no strip bit8}
                        end;
              auxdata:  packed record
                          spare7,spare6,spare5,spare4,
                          spare3,spare2,spare1,spare0: boolean;
                        end;
              maxserports : 0..7;
              expanstwo:  array [0..9] of integer;  {spares}
              auxcrtinfo: packed record
                            verdlaychar: char;
                            killqueue: char
                          end;
              curtime : dp_integer; {hi,lo: integer}
              miscinfo: packed record
                          nobreak,stupid,slowterm,
                          hasxycrt,haslccrt,
                          nointerrupts,hasclock: boolean;
                          userkind:(normal, aquiz, booker, pquiz)
                        end;
              crttype: integer;
              crtctrl: packed record
                         rlf,ndfs,eraseeol,eraseeos,home,escape: char;
                         backspace: char;
                         fillcount: 0..255;
                         clearscreen, clearline: char;
                         prefixed: packed array [0..8] of boolean
                       end;
              crtinfo: packed record
                         width,height: integer;
                         right,left,down,up: char;
                         badch,chardel,stop,break,flush,eof: char;
                         altmode,linedel: char;
                         backspace,etx,prefix: char;
                         prefixed: packed array [0..13] of boolean
                       end );
            end { SYSCOM };
```

Notes:

- "WE ASSUME BACKWARD FIELD ALLOCATION": the compiler allocates record fields **backward**,
  so the last-declared field has the lowest address. ⚠ This matters when Z80 code (the BIOS,
  ZMCOM) or a host tool reads SYSCOM by offset; settle the direction against a memory dump of a
  booted system before relying on any offset.
- `^static_config` is printed for `^static_configrec` (a slip; the type is declared in 5.5).
- `boot_config` (variant 1) and `config` (variant 2) share the first word: at boot it points at
  the configuration record as read from disk, later at the in-memory `configrec`.
- `miscinfo`, `crtctrl` and `crtinfo` describe the console terminal. In UCSD systems the
  `SETUP` utility saves them in `SYSTEM.MISCINFO`, which the OS reads at boot (the user set
  VT52 arrow keys this way on the real machine). ⚠ That this H3 layout matches the user's
  `SYSTEM.MISCINFO` byte for byte is not checked.

### 5.7 File information block (I-10)

```pascal
{ File Information Block and related types
  Needs definition of vid, tid, direntry, fblksize.
  The needed definitions are all in t_directry. }

                                       { FILE INFORMATION }
closetype = (cnormal, clock, cpurge, ccrunch);
fibp = ^fib;

fib = record
        fwindow: windowp;     { USER WINDOW...F^, USED BY GET-PUT }
        feof,feoln: boolean;
        fstate: (fjandw,fneedchar,fgotchar);
        frecsize: integer;    { IN BYTES...0=>BLOCKFILE, 1=>CHARFILE }
        case fisopen: boolean of
          true: (fisblkd: boolean;   { FILE IS ON BLOCK DEVICE }
                 funit: integer;     { PHYSICAL UNIT # }
                 fvid: vid;          { VOLUME NAME }
                 freptcnt,           { # TIMES F^ VALID W/O GET }
                 fnxtblk,            { NEXT REL BLOCK TO IO }
                 fmaxblk: integer;   { MAX REL BLOCK ACCESSED }
                 fmodified:boolean;  { SET NEW DATE IN CLOSE }
                 fheader: direntry;  { COPY OF DISK DIR ENTRY }
                 flock : semaphore;  { File access lock. }
                 case fsoftbuf: boolean of { DISK GET-PUT STUFF }
                   true: (fnxtbyte,fmaxbyte: integer;
                          fbufchngd: boolean;
                          fbuffer: packed array [0..fblksize] of char))
      end { FIB } ;
```

### 5.8 User work file record and system constants (I-11, I-12)

```pascal
{ User Work file stuff
  Needs definition of fibp, vid, tid.
  The definitions can be found in t_directry, t_fileinfo. }
inforec = record
            symfibp,codefibp: ^fib;         { WORKFILES FOR SCRATCH }
            errsym,errblk,errnum: integer;  { ERROR STUFF IN EDIT }
            slowterm,stupid: boolean;       { STUDENT PROGRAMMER ID!! }
            altmode: char;                  { WASHOUT CHAR FOR COMPILER }
            gotsym,gotcode: boolean;        { TITLES ARE MEANINGFUL }
            workvid,symvid,codevid: vid;    { PERM&CUR WORKFILE VOLUMES }
            worktid,symtid,codetid: tid;    { PERM&CUR WORKFILES TITLE }
          end { INFOREC } ;

{ System definitions }
CONST
  osversion  = '[H3]';  {common os base}
  useCDint   = false;   { whether to use carrier det interrupt for remote I/O}
  mmaxint    = 32767;   { maximum integer value }

  has_timed_out = -1;
  not_in_time_q = -2;

  firstsysseg  = 0;
  maxsysseg    = 127;
  firstuserseg = 128;
  maxuserseg   = 255;
  maxsubseg    = 15;

             { THESE CONSTANTS USED BY I/O ROUTINES }

  cmaxport   =   1; {0..1}
  maxcard    =   1; {0..1}
  maxretry   =   4; { retry count for disk drivers }
  mievalue   =   1; { interrupt enable value }
  eol        =  13; { end-of-line ...ASCII cr }
  dle        =  16; { blank compression code }
  maxq       =  79; { type-ahead queue index limit }
  maxqpl     =  80; { type-ahead queue length }
  xonqavail  =  60; { number of characters available before xon sent }
  xoffqavail =  20; { number of characters available before xoff sent }
  xon        =  17; { control-Q transmitt on }
  xoff       =  19; { control-S transmitt off }

  hdiskaddr  = -912; { FC70 Winchester address }
  cond_hog   = false;

  hiiopriority  = 250; { kbddriver (serial input) processes     }
  midiopriority = 245; { disk in/out, parallel out, serial out   }
  lowiopriority = 240; { lowest priority for system processes   }

TYPE
  byte     = 0..255;

  iorsltwd = (inoerror,ibadblock,ibadunit,ibadmode,itimeout,
              ilostunit,ilostfile,ibadtitle,inoroom,inounit,
              inofile,idupfile,inotclosed,inotopen,ibadformat,
              iwriteprot);

                                { COMMAND STATES...SEE GETCMD }
  cmdstate = (haltinit,debugcall,
              uprognou,uproguok,sysprog,
              componly,compandgo,compdebug,
              linkandgo,linkdebug);

                                { CODE FILES USED IN GETCMD }
  sysfile  = (adacomp,compiler,editor,filer,linker);

  integerp  = ^integer;
  bytearray = packed array [0..0] of byte;
  codeseg   = record case boolean of
                true:  (int: packed array [0..0] of integer);
                false: (byt: bytearray);
              end;

  sibp   = ^sib;
  sibvec = array [0..0] of sibp;
  sib    = record { segment info block }
             segbase: ^codeseg; { memory address of seg }
             segleng: integer;  { # words in segment }
             segrefs: integer;  { active calls - microcode maintained }
             segaddr: integer;  { absolute disk address }
             segunit: integer;  { physical disk unit }
             prevsp:  integerp; { SP saved by getseg for relseg cut back }
           end { sib } ;
```

Notes:

- **Segments 0..127 are system segments, 128..255 user segments**, matching `getsegb` in Table
  C-3 (section 2.6): numbers below 128 index `ssv`, the rest the task's `sibs`. `maxsubseg = 15`
  is the 16 segments one code file can hold.
- `segrefs` is "microcode maintained": the call and return operators keep it (section 2.19).
- `iorsltwd` is the **IORESULT** list, 0 = no error up to 15 = write-protected. Compare the
  Pascal-100 manual, appendix H B.3 (I/O results).
- The three I/O priorities (250/245/240) are the TIB `prior` values of the OS's driver tasks.
  Interrupt-driven input runs at the highest one.
- ⚠ `hdiskaddr = -912 { FC70 Winchester address }`, `cmaxport`, `maxcard` and `mievalue` are WD
  hardware (SB1600/ME1600). The Pascal-100 does its I/O through the Z80 BIOS instead; whether its
  III.0 build still carries these constants is not known.
- `sibvec = array [0..0]` here (an open array, indexed past its bound), where C-34 declares
  `[0..127]`. Same object.

### 5.9 Mark stack, task, and memory records (I-13)

```pascal
mscwp = ^mscw;
mscw  = packed record  { mark stack control word }
          msstat: mscwp;   { lexical parent pointer }
          msdynl: mscwp;   { ptr to caller's mscw }
          msipc:  integer; { byte index in return code seg }
          msseg:  byte;    { seg # of caller code }
          msflag: byte
        end { mscw } ;

semp  = ^semtrix;
tibp  = ^tib;
tib   = record { Task Information Block }
          regs: packed record
                  waitq: tibp;        { QUEUE LINK FOR SEMAPHORES }
                  prior: byte;        { TASK'S CPU PRIORITY }
                  flags: byte;        { STATE FLAGS...NOT DEFINED YET }
                  splow: integerp;    { LOWER STACK POINTER LIMIT }
                  spupr: integerp;    { UPPER LIMIT ON STACK }
                  sp: integerp;       { ACTUAL TOP-OF-STACK POINTER }
                  mp: mscwp;          { ACTIVE PROCEDURE MSCW PTR }
                  bp: mscwp;          { BASE ADDRESSING ENVIRONMENT PTR }
                  ipc: integer;       { BYTE PTR IN CURRENT CODE SEG }
                  segb: ^codeseg;     { PTR TO SEG CURRENTLY RUNNING }
                  hangp: semp;        { WHICH TASK IS WAITING ON }
                  iorslt : iorsltwd;  { Result of last I/O call. }
                  sibs: ^sibvec       { ARRAY OF SIBS FOR 128..255 }
                end { REGS } ;
          maintask  : boolean;        { true if tib is root task (os tib) }
          startmscw : mscwp;          { top mscw in task's stack }
          nexttib   : tibp            { next pointer for list starting with}
        end { TIB } ;

decmax   = integer[36];
longtrix = record case integer of
             0: (intar: array [0..0] of integer);
             1: (BCDar: packed array [0..0] of 0..15)
           end {longtrix};

bytetrix = record case integer of
             1: (int  : integer);
             2: (byte : packed array[0..1] of 0..255)
           end;

memtrix  = record case integer of
             1 : ( addr : integer );
             2 : ( loc  : integerp );
             3 : ( wp   : windowp );
             4 : ( int  : integer );
             5 : ( pack : packed array [0..1] of byte );
           end;
```

Notes:

- The **OS's TIB** is the microcode's 12 register words (`regs`, section 1) followed by three OS
  fields: `maintask`, `startmscw`, `nexttib`. The Pascal-100 manual's appendix F has the first
  two but not `nexttib`.
- `semp = ^semtrix`: the semaphore record type that section 1 found undeclared. `semtrix` is
  declared on I-16 (section 5.12).
- **`msseg` and `msflag`.** They share word 3 of the mark stack, `msseg` declared first. Which
  byte that makes it depends on how the compiler packs fields into a word, which is not stated.
  The evidence: records are allocated forward (5.14), byte arrays put element 0 in the low
  byte (C-37, `LDB`), and most device records put the first-declared bit at bit 0 (5.12). So
  `msseg` is **probably the low byte**. ⚠ Still to confirm from a mark stack in memory.
- `memtrix` here is the OS's own overlay type, not the microcode metalanguage's `memtrix` of
  section 1 (same name, different record).

### 5.10 Clock, segment page and copier records (I-14)

```pascal
un_signed = record case integer of
              1 : ( i : integer );
              2 : ( p : integerp )
            end;

clocknode = record
              delay_sem : semp;         { semaphore to signal to awaken }
              timed_out : ^boolean;
              time_out  : dp_integer;   { time to be awaken }
              time_link : ^clocknode;   { points to next clocknode }
            end;

segrange    = firstsysseg..maxuserseg;
segsubrange = 0..maxsubseg;
segpage = record
            diskinfo    : array [segsubrange] of
                            record
                              codeleng,
                              codeaddr : integer
                            end;
            segname     : array [segsubrange] of alpha;
            segkind     : array [segsubrange] of
                            (linked, hostseg, segproc, unitseg, seprtseg);
            textaddr    : array [segsubrange] of integer;
            seginfo     : array [segsubrange] of
                            packed record
                              segnum : segrange;
                              codeversion : 0..255
                            end;
            notice      : string[79];
            codekind    : (static, vectored);
            lastseg     : integer;
            lastcodeblk : integer;
            filler      : packed array [1..56] of char
          end;

copierrec = record
              request_rendezvous : semaphore;
              end_rendezvous     : semaphore;
              copierbusy         : boolean;
              killcopy           : boolean;
              sunit              : integer;
              dunit              : integer
            end;
```

`segpage` is **block 0 of a code file**, the segment dictionary: for each of up to 16 segments,
its length and block address, name, kind, text address, number and code version, then a
copyright notice. This is what the OS loads `SYSTEM.PASCAL` by, and what a host tool needs to
list the segments of a code file.

⚠ Size: with one word per `segkind` and `seginfo` entry and 40 words for `notice`
(`string[79]` = length byte + 79 chars), the fields add up to 16×2 + 16×4 + 16 + 16 + 16 + 40 +
1 + 1 + 1 + 28 = **215 words**, not the 256 words of a 512-byte block. Either some field is
larger than assumed or the rest of the block is unused. Settle from block 0 of a code file on
the user's disks before writing a reader.

### 5.11 WD hardware status words (I-15) — WD machines only

```pascal
sys_control_word = packed record
                     filler    : 0..255;   { bits 7..0   }
                     baud_rate : 0..15;    { bits 11..8  }
                     drb       : 0..15     { bits 15..12 }
                   end;

sys_stat_word =
  packed record
    case integer of
      1: (int : integer);
      2: (is_G_board,          { bit 15, G Board Identification            }
          bit14, bit13, bit12, bit11, bit10,
                               { bits 10-14, ????                          }
          PERR,                { bit 9, Parity Error                       }
          bit8,                { bit 8                                     }
          MEMRTO,              { bit 7, Memory Reply Timeout               }
          INTEN,               { bit 6, Interrupt Enable                   }
          EOB,                 { bit 5, EOB                                }
          DINTR,               { bit 4, DINTR                              }
          DDEN,                { bit 3, Double Density Enable              }
          BFFD,                { bit 2, Boot From Floppy Disk              }
          eight_inch,          { bit 1, indicates if 8" or 5.25" floppy    }
          IACKRTO              { bit 0, Interrupt Acknowledge Reply Timeout}
            : boolean)
  end;
```

These are WD SB1600 board registers (`drb` is the diagnostic result bits of section 4.1); the
Pascal-100 has neither. **Useful as a packing rule:** declared first = **highest** bits here
(`is_G_board` is bit 15, `IACKRTO` bit 0), and in `sys_control_word` declared first = **lowest**
bits (`filler` is bits 7..0). ⚠ The two records on one page contradict each other on packing
order, so neither settles `msseg` (section 5.9). Only a memory dump will.

### 5.12 Semaphores and device registers (I-16 to I-18)

"THE REMAINING TYPE DECLARATIONS ARE FOR THE CONTROL OF DISK, SERIAL, AND PARALLEL I/O"

```pascal
semtrix = record case integer of
            0: (sem: semaphore);
            1: (fakesem: record
                           count: integer; { outstanding signals }
                           waitq: tibp     { task queue }
                         end);
          end { sem } ;

{ for devices that use same reg for stat and cmd }
statcmdrec = record case integer of
               1 : ( bit0 : boolean );    { efficent way of testing bit 0 }
               2 : ( command : integer );
               3 : ( status : packed array[0..7] of boolean );
             end;  { for devices that use same reg for stat and cmd }

dstatrec = packed record case integer of {status / command reg}
             1: (command : integer);
             2: (bow     : boolean;
                 dint    : boolean;
                 toi     : boolean;
                 tczi    : boolean;
                 iom     : boolean;
                 hbus    : boolean;
                 aece    : boolean;
                 busy    : boolean)
           end;

dmacntrbits = (RUN,DIE,TOIE,TCIE,IOM,HBUS,AECE,unused);
dmarec = record   { DMA device register uses for both floppy and
                    winchester disk accesses }
           dcontrol : set of dmacntrbits;
           dstatus  : dstatrec;    {status / command reg}
           trcountl : integer;     {transfer count low order byte}
           trcounth : integer;     {transfer count high order byte}
           bufaddl  : integer;     {transfer buffer address low byte}
           bufaddh  : integer;     {transfer buffer address high byte}
           memex    : integer;
           intbase  : integer
         end;

fscomrec = packed record case integer of {status / command reg}
             { variants 0..4 represent command bits }
             0: (allbitsint: integer);
             1: (command   : set of 0..7);
             2: (commandint: 0..255);
             3: (drivesel  : packed array[-8..7{0..3 are drives}] of boolean);
             4: (filler0   : boolean;
                 filler1   : boolean;
                 filler2   : boolean;
                 filler3   : boolean;
                 filler4   : boolean;
                 filler5   : boolean;
                 filler6   : boolean;
                 filler7   : boolean;
                 filler8   : boolean;
                 filler9   : boolean;
                 filler10  : boolean;
                 filler11  : boolean;
                 unused1   : boolean;
                 unused2   : boolean;
                 densitysel: boolean;
                 sidesel   : 0..1);

             { the remaining variants represent status bits }
             5: (busy       : boolean;
                 index      : boolean;
                 track0     : boolean;
                 bit3       : boolean;
                 seekerror  : boolean;
                 headloaded : boolean;
                 bit6       : boolean;
                 notready   : boolean);
             6: (bit0         : boolean;
                 drq          : boolean;
                 lostdata     : boolean;
                 crcerror     : boolean;
                 rnf          : boolean;
                 writefault   : boolean;
                 writeprotect : boolean;
                 bit7         : boolean)
           end;

floppyrec = record {floppy device regiseter}
              fstatcom : fscomrec; {floppy status command register}
              track    : integer;  {current track number. FDC updates this}
              sector   : integer;  {sector to read or write}
              data     : integer;  {for track to seek to. also data to read
                                    or write if no DMAC used}
              eobdintr : sys_stat_word; {has eob and dintr bits for modular}
              intprior : integer;
              flintbase: integer;
              filler   : integer;
              dma      : dmarec {floppy dma controller follows floppy regs}
            end;

taskfyle = packed record                       { for mass assignment }
             curblock    : integer;
             case integer of
               1 : ( curcylinder : integer );  { working copy here }
               2 : ( lo_cylinder : byte;
                     temp_hi_cyl : byte;
                     hi_cylinder : integer;
                     curhead     : 0..7;
                     curdrive    : 0..3;
                     secsize     : 0..3;
                     zero_bits   : 0..511 ); { these bits MUST be zeroed }
           end;

winchrec = record
             wdatareg   : integer;
             werrprecom : statcmdrec;
             wsectorcnt : integer;
             case integer of
               1 : ( taskfile : taskfyle );
               2 : ( wsectornum : integer;
                     wcyllow    : integer;
                     wcylhigh   : integer;
                     wsdh       : integer;
                     wstatcom   : packed record case integer of
                                    1: (command      : integer);
                                    2: (error        : boolean;
                                        unused1      : boolean;
                                        unused2      : boolean;
                                        datarequest  : boolean;
                                        seekcomplete : boolean;
                                        writefault   : boolean;
                                        ready        : boolean;
                                        busy         : boolean)
                                  end;
                     dma        : dmarec)
           end;
```

Notes:

- `semtrix` is the semaphore as a record, **count first, then the wait queue**, as Table C-3's
  `sem_obj` uses it (`sem.count`, `sem.waitq`). This settles section 1's undeclared `semp`.
- The rest is **WD hardware** (the SB1600's floppy controller, DMA controller and Winchester
  task file), not used by the Pascal-100. One thing it shows about packing: in `fscomrec`
  variant 5 the floppy controller's status bits are declared bit 0 first (`busy` = bit 0 of a
  WD179x status register, `notready` = bit 7). So in these device records **declared first =
  lowest bit**. The same holds for `statcmdrec.bit0`. That agrees with `sys_control_word` and
  disagrees with `sys_stat_word` (section 5.11). ⚠ The direction for `msseg` stays open.

### 5.13 Disk and I/O request blocks (I-19, I-20)

```pascal
{ DISKBOARD AND DISKCONTROL ARE THE COMMUNICATION LINK BETWEEN DISKIO
  AND THE FLOPPY AND WINCHESTER DRIVERS }

diskboard = record {disk control block for one floppy / winchester board}
              disksem   : semaphore; {attached to DMA/floppy interrupt}
              disktrix  : record case integer of {floppy/DMA device regs}
                            0 : (address : integer);
                            1 : (floppy  : ^floppyrec);
                            2 : (hdisk   : ^winchrec)
                          end;
              dintreob  : ^sys_stat_word; { points to register on floppy
                                            board if modular otherwise
                                            it points to the system
                                            status word }
              {DENSITY reflects density last time drive was accessed}
              curdrive  : 0..3;
              density   : packed array[0..3] of boolean;
            end;

modes = (readmode, writemode, clearmode);
diskcontrol = record {floppy control block for all floppy boards}
                disklock  : semtrix;  {limits use of floppies to one at a time}
                unitselect: integer;  { reflects unit number }
                buffer    : windowp;  { transfer buffer pointer }
                boardnum  : 0..maxcard; { floppy board # for this access}
                board     : array[0..maxcard] of ^diskboard; {fields unique to
                                                             indiviual boards}
                mode      : modes;
                flags     : integer;   {2 = physical sector mode}
                trcount   : integer;   {bytes to transfer}
                inx       : integer;   {offset in fa^ to start transfer}
                startblock: integer;   {block to start transfer}
                ioerror   : iorsltwd;  {error result of tranfer}
                haswork   : semaphore; {signals driver to begin transfer}
                ready     : semaphore  {signaled when transfer complete}
              end;

{ COMMUNICATION LINK BETWEEN UNITREAD/UNITWRITE AND
  THE PARALLEL AND SERIAL DRIVERS }

iorequest = record {Comm link between I/O drivers and unitread/unitwrite}
              iohavework : semaphore; {tells driver to begin transfer}
              ioready    : semaphore; {locks port to one at a time use}
              iodone     : semaphore; {signalled when I/O complete}
              iounit     : integer;   {unit number for I/O}
              iowindowp  : windowp;   {points to buffer for transfer}
              ioflags    : integer;   {transfer mode bits}
              iobytes    : integer;   {number of bytes left to transfer}
              ioinx      : integer    {offset in iowindowp for tranfer}
            end;

          {**********    PARALLEL I/O TYPES    **********}

cards = 0..maxcard;

paralrec = record {parallel port registers}
             porta    : statcmdrec;
             portb    : integer;
             portc    : statcmdrec;
             pcontrol : integer;
           end;

parcontrol = record {control block for parallel communication}
               paraltrix : record case integer of {parallel device registers}
                             1 : (pdevadd  : integer);
                             2 : (parallel : ^paralrec)
                           end;
               paronline : boolean; {true if unitclear found device online}
               parsem    : semaphore; {attached to parallel interrupt}
               request   : iorequest {comm link to parallel output driver}
             end;
```

Notes:

- These blocks are how the OS talks to its driver **tasks**: a request is posted, the driver
  task is woken with `SIGNAL(haswork)` / `SIGNAL(iohavework)`, and the caller `WAIT`s on
  `ready` / `iodone`. A device interrupt **signals a semaphore** (`disksem`, `parsem`,
  "attached to ... interrupt"), the mechanism of Table C-3 label 6. On the Pascal-100 the
  devices are behind the Z80 BIOS, but the same task/semaphore structure is how a hang like the
  SD-in-B case can appear: a task `WAIT`s on a semaphore that is never signalled.
- ⚠ I-20's scan carries **handwritten word offsets** beside `iorequest`'s fields:
  0, 2, 4, 6, 7, 8, 9, 10. A semaphore is 2 words (5.12), so these are forward offsets from the
  start of the record: `iohavework` at word 0. If they are right, **ordinary records are
  allocated forward**, and SYSCOM's "backward field allocation" (5.6) is particular to SYSCOM.
  The annotation is a reader's, not WD's; treat it as a lead.

### 5.14 Serial I/O (I-21, I-22)

```pascal
          {**********    SERIAL I/O TYPES    **********}

ports = 0..cmaxport;

cntrl1bits = (DTR,RTS,RE,PE,ECHO,STOP1,BRK,NRML);
serialrec = record {serial port registers}
              data     : integer;     {data to be read or written}
              status   : packed record case integer of
                           1: (command   : integer);
                           2: (bit0      : boolean);
                           3: (thre      : boolean;
                               dr        : boolean;
                               oe        : boolean;
                               parityerr : boolean;
                               fe        : boolean;
                               cd        : boolean;
                               dsr       : boolean;
                               dsc       : boolean)
                         end;
              control2 : integer;     {control register 2}
              control1 : set of cntrl1bits; {control register 2}
              filler   : integer;
              baudrate : integer      {baudrate select on ME1600s}
            end;

auxsercntrl = record {control block for serial communication}
                qlock      : semaphore; {locks use of rear and front}
                havch      : semaphore; {kbddriver signals when it has char}
                writesem   : semaphore; {attached to serial output interrupt}
                writebell  : semaphore; {tells bellprocess to ring bell
                                         for input buffer overflow}
                readsem    : semaphore; {attached to serial input interrupt}
                rear       : integer;   {points to rear of input queue}
                front      : integer;   {points to front of input queue}
                chq        : packed array [0..maxq] of byte; {input queue}
                serialtrix : record case integer of {serial device registers}
                               0: (sdevadd: integer);
                               1: (serial: ^serialrec)
                             end;
                stst       : semaphore; {stopped output wait on this}
                stwaitno   : integer;   {number of tasks w/ output stopped}
                fflag      : boolean;   { true means flush output }
                sflag      : boolean;   { true means freeze output }
                ioerror    : iorsltwd;  { kind of error during I/O }
                xofflag    : boolean    { true means xoff sent }
              end;

sercontrol = record {these logically belong with auxsercntrl, but
                     would upset sequence of U- global variables}
               avail     : integer;   { bytes available in input queue }
               statusq   : packed array [0..maxq] { true if error in }
                             of boolean;          { read             }
               sendxoff  : semaphore; { tells writexoff to send an xoff
                                        char if xon/xoff enabled       }
               lport     : ports;     { port number for this record    }
               cardetsem : semaphore; { signaled when carrier goes high}
               seronline : boolean;   { port online when last checked  }
               request   : iorequest; { comm link between to driver    }
               auxport   : ^auxsercntrl {more fields like those in this
                                          record                       }
             end;

arraysercntrl = array[0..0] of sercontrol; {actual instance of this type
                                            may be less than cmaxport long}
```

Notes:

- **The console input path.** A serial input interrupt signals `readsem`; the keyboard driver
  task (priority `hiiopriority` = 250, section 5.8) puts the character into the 80-byte queue
  `chq` (`rear`/`front`) and signals `havch`; XON/XOFF is sent at 60 / 20 characters free
  (`xonqavail`, `xoffqavail`). `serialrec` is WD's own UART; on the Pascal-100 the characters
  come through the Z80 BIOS instead.
- **Field allocation, confirmed by arithmetic.** The scan again carries handwritten word offsets.
  On I-22: `avail` 0, `statusq` 1, `sendxoff` 6, `lport` 8, `cardetsem` 9, `seronline` 11,
  `request` 12, `auxport` 23. They agree with forward allocation exactly: `statusq` is 80
  packed booleans = 5 words (1 + 5 = 6), a semaphore 2 words, and an `iorequest` 11 words
  (12 + 11 = 23). On I-21, `chq` (80 packed bytes = 40 words) runs from word 12 to the
  `serialtrix` written as 52. So ordinary records **are allocated forward**, first field at the
  lowest address, and a packed byte array packs two bytes to a word.

### 5.15 The OS global variables (I-23)

```pascal
VAR
  syscom    : ^syscomrec;        { MAGIC PARAM...SET UP IN BOOT }
  gfiles    : array [0..5] of fibp; { not used anywhere GLOBAL FILES, 0=INPUT, 1=OUTPUT }
  userinfo  : inforec;           { WORK STUFF FOR COMPILER ETC }
  ostibp    : tibp;              { taskinfo block of op sys prog }
  emptyheap : ^integer;          { HEAP MARK FOR MEM MANAGING }
  inputfib,outputfib,            { CONSOLE FILES...GFILES ARE COPIES }
  systerm,swapfib: fibp;         { CONTROL AND SWAPSPACE FILES }
  syvid,dkvid: vid;              { SYSUNIT VOLID & DEFAULT VOLID }
  thedate   : daterec;           { TODAY...SET IF FILER OR SIGN ON }
  state     : cmdstate;          { FOR GETCOMMAND }
  heapinfo  : record             { heap management }
                lock: semaphore;
                topmark,
                heaptop: integerp
              end { heapinfo } ;
  taskinfo  : record             { stuff for task management }
                lock: semaphore;
                taskdone: semaphore; { signalled when task stops }
                ntasks: integer      { decremented when task stops }
              end { taskinfo } ;
  ipot      : array [0..4] of integer; { INTEGER POWERS OF TEN }
  filler    : string[41];        { NULLS FOR CARRIAGE DELAY }
  digits    : set of '0'..'9';
  pl        : string;            { prompt line }
  chainname : string[23];        { chainer sets this - length > 0 means }
                                 { next getcmd executes chainname }
  oldunitable : array [unitnum] of unitentry; {27 unit descriptors to be
                                     compatible with old programs.
                                     Remaining unit descriptors
                                     are in unitable which has
                                     pointers to these entries as
                                     well}
  filename  : array [sysfile] of string[23]; {'system.filer',etc.}
  topofsibs : ^integer;
  safediskmode : boolean ;
  oldport   : array [ports] of auxsercntrl; {two serial port control
                                     blocks to be compatible with
                                     old programs. Remaining control
                                     blocks pointed to by serport
                                     which also points at these two
                                     blocks}
  modular   : boolean;           {is currently an ME1600 computer}
  maxunit   : integer;           {largest unit currently available}
  unitable  : ^arrayunitable;    {name of vol, unittype, etc.}

  {..........Variable access by system U- programs ends here..........}
```

Notes:

- These are the **global variables of segment 0** (the OS), in order; user programs that are
  compiled as "U-" (system) programs reach them by offset, which is why the order is fixed.
  They live in the OS's global data area, the activation record that `bp` (BASE) points at.
- `syscom` is the first global and points at the SYSCOM record of section 5.6. `ostibp` is the
  OS task's TIB.
- `pl`, the **prompt line**, is what the III.0 command line prints at the top of the screen:
  the target of Stage 5's boot gate.
- The handwritten offsets on this page are partly legible only (for example `oldunitable` 162,
  `filename` 330, `oldport` 392, `modular` 512); they are not transcribed as data.
- ⚠ `modular` "is currently an ME1600 computer", `oldport` and the serial records are WD
  hardware. How the Pascal-100's III.0 build maps console and disk I/O onto the Z80 BIOS is
  described in the Digicomp manual (ZMCOM, section 7 of its reference), not here.

---

## 6. Table C-1: P-machine opcodes (C-1 to C-13)

Printed page C-*n* is PDF page *n* + 374, as for Table C-3.

C-1, as printed: "Instructions are one byte long, followed by zero-to-three parameters. Most parameters
specify one word of information and are one of five basic types." The five types (**UB**, **SB**,
**DB**, **B**, **W**) are given in section 2's introduction. "These mnemonics are intended only for
further understanding of P-code. Neither the Western Digital Corporation nor the University of
California at San Diego provide P-code assembler software."

Each row gives the opcode in decimal as printed, with the hex added. The description is C-1's own
wording (Tos = top of stack, Tos-1 the word under it). ✓ in the last column means C-1 and Table C-3
agree on opcode and parameters; any difference is written out.

### 6.1 One-word loads and stores (C-2, C-3, C-4)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| SLDC | 0..31 | 00..1F | | Short Load Word Constant (Value 0-31). Pushes the opcode, with high byte zero, onto the stack. | ✓ |
| LDCN | 152 | 98 | | Load Constant Nil (FC00). Pushes nil onto the stack. | ✓ |
| LDCB | 128 | 80 | UB | Load Constant Byte. Pushes UB, with high byte zero, onto the stack. | ✓ |
| LDCI | 129 | 81 | W | Load Constant Word. Pushes W onto the stack. | ✓ |
| LCA | 130 | 82 | B | Load Constant Address. Pushes the word address of the constant, with offset B in the constant word block, onto the stack. | ✓ |
| SLDL1..16 | 32..47 | 20..2F | | Short Load Local Word. Fetches the word with offset 1..16 in the local activation record and pushes it on the stack. | ✓ |
| LDL | 135 | 87 | B | Load Local Word. Fetches the word with offset B in the local activation record and pushes it on the stack. | ✓ |
| LLA | 132 | 84 | B | Load Local Address. Fetches address of the word with offset B in the local activation record and pushes it on the stack. | ✓ |
| STL | 164 | A4 | B | Store Local. Stores Tos into the word with offset B in the local activation record. | ✓ |
| SLDO1..16 | 48..63 | 30..3F | | Short Load Global Word. Fetches the word with offset 1..16 in the base activation record and pushes it on the stack. | ✓ |
| LDO | 133 | 85 | B | Load Global Word. Fetches the word with offset B in the base activation record and pushes it on the stack. | ✓ |
| LAO | 134 | 86 | B | Load Global Address. Pushes the word address of the word with offset B in the base activation record. | ✓ |
| SRO | 165 | A5 | B | Store Global Word. Stores Tos into the word with offset B in the base activation record. | ✓ |
| LOD | 137 | 89 | DB,B | Load Intermediate Word. DB indicates the number of static links to traverse to find the activation record to use. B is the offset within the activation record. | ✓ |
| LDA | 136 | 88 | DB,B | Load Intermediate Address. | ✓ |
| STR | 166 | A6 | DB,B | Store Intermediate Word. | ✓ |
| STO | 196 | C4 | | Store Indirect. Tos is stored into the word pointed to by Tos-1. | ✓ |
| LDE | 154 | 9A | UB,B | Load Word Extended. Fetches the word with offset B in segment number UB and pushes it on the stack. | ✓ |
| LAE | 155 | 9B | UB,B | Load Address Extended. | ✓ |
| STE | 217 | D9 | UB,B | Store Word Extended. | ✓ |

### 6.2 Multiple-word loads and stores, byte arrays (C-4)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| LDC | 131 | 83 | B,UB | Load Multiple Word Constant. B is the offset within the constant word block, and UB is the number of words to load. Push the block onto the stack. | ✓ |
| LDM | 208 | D0 | UB | Load Multiple Words. Tos is a pointer to the beginning of a block of UB words. Push the block onto the stack. | ✓ |
| STM | 142 | 8E | UB | Store Multiple Words. Tos is a block UB words, Tos-1 is a word pointer to a similar block. Transfer the block from the stack to the destination block. | ✓; settles the C-3 ⚠ (section 2.7): the pointer is **under** the block, `(sp + UB)^.p` |
| LDB | 167 | A7 | | Load Byte. Push the byte (after zeroing high byte) pointed to by byte pointer Tos. | ✓ |
| STB | 200 | C8 | | Store Byte. Store byte Tos into the location specified by Byte Pointer Tos-1. | ✓ |

### 6.3 Record and array indexing and assignment (C-5)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| MOV | 197 | C5 | B | Move Words. Tos is a source pointer to a block of B words, Tos-1 is a destination pointer to a similar block. Transfer the block from the source to the destination. | ✓ |
| SIND0..7 | 120..127 | 78..7F | | Short Index and Load Word. Indexes the word pointer Tos by 0..7 words, and pushes the word pointed to by the result. | ✓ |
| IND | 230 | E6 | B | Static Index and Load Word. Index the word pointer Tos by B words, and push the word pointed to. | ✓ |
| INC | 231 | E7 | B | Increment Field Pointer. The word pointer Tos is indexed by B words and the resultant pointer is pushed. | ✓ |
| IXA | 215 | D7 | B | Index Array. Tos is an integer index, Tos-1 is the array base word pointer, and B is the size (in words) of an array element. A word pointer to the indexed element is pushed. | ✓ |
| IXP | 216 | D8 | UB1,UB2 | Index Packed Array. Tos is an integer index, Tos-1 is the array base word pointer. UB1 is the number of elements-per-word, and UB2 is the field-width (in bits). Compute and push a packed field pointer. | ✓ |
| LDP | 201 | C9 | | Load A Packed Field. Push the field described by the packed field pointer, Tos. | ✓; supports the C-3 ⚠ (section 2.9): the field is fetched **through** the pointer |
| STP | 202 | CA | | Store Into A Packed Field. Tos is the data, Tos-1 is a packed field pointer. Store Tos into the field described by Tos-1. | ✓ |

### 6.4 Logicals and integers (C-6, C-7)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| LAND | 161 | A1 | | Logical AND. AND Tos into Tos-1. | ✓ |
| LOR | 160 | A0 | | Logical OR. OR Tos into Tos-1. | ✓ |
| LNOT | 229 | E5 | | Logical NOT. Take one's complement of Tos. | ✓ |
| BNOT | 159 | 9F | | Boolean NOT. | ✓, but 159 is also `RBP` on C-13 (section 2.22) |
| LEUSW | 180 | B4 | | Compare Unsigned Word <=. Compare unsigned word of Tos-1 to unsigned word of Tos and push true or false. | ✓ |
| GEUSW | 181 | B5 | | Compare Unsigned Word >=. Compare unsigned word of Tos-1 to unsigned word of Tos and push true or false. | ✓ |
| ABI | 224 | E0 | | Absolute Value of Integer. Take absolute value of integer Tos. | ✓ |
| NGI | 225 | E1 | | Negate Integer. Take the two's complement of Tos. | ✓ |
| DUP1 | 226 | E2 | | Copy Integer. Duplicate one word on Tos. | ✓ |
| ADI | 162 | A2 | | Add Integers. Add Tos and Tos-1. | ✓ |
| SBI | 163 | A3 | | Subtract Integers. Subtract Tos from Tos-1. | ✓ |
| MPI | 140 | 8C | | Multiply Integers. Multiply Tos and Tos-1. | ✓ |
| DVI | 141 | 8D | | Divide Integers. Divide Tos-1 by Tos and push quotient. | ✓ |
| MODI | 143 | 8F | | Modulo Integers. Divide Tos-1 by Tos and push the remainder. | ✓ |
| CHK | 203 | CB | | Check Against Subrange Bounds. Insure that Tos-1 <= Tos-2 <= Tos, leaving Tos-2 on the stack. If conditions are not satisfied a run-time error occurs. | ✓ |
| EQUI | 176 | B0 | | Compare Integer =. Compare Tos-1 to Tos and push true or false. | ✓ |
| NEQI | 177 | B1 | | Compare Integer <>. Compare Tos-1 to Tos and push true or false. | ✓ |
| LEQI | 178 | B2 | | Compare Integer <=. Compare Tos-1 to Tos and push true or false. | ✓ |
| GEQI | 179 | B3 | | Compare Integer >=. Compare Tos-1 to Tos and push true or false. | ✓ |

`NEQI`, `LEQI` and `GEQI` are printed "Compare Tos-1 to to Tos"; the doubled "to" is dropped
above.

### 6.5 Reals, sets and byte arrays (C-7, C-8, C-9)

"Reals (All Over/Underflows Cause a Run-Time Error)"

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| FLT | 204 | CC | | Float Top-of-Stack. The integer Tos is converted to a floating point number. | ✓ |
| TNC | 190 | BE | | Truncate Real. The real Tos is truncated and converted to an integer. A run-time error results if the real is outside of permissible integer values. | ✓; C-3 does not mention this error |
| RND | 191 | BF | | Round Real. The real Tos is rounded and converted to an integer. A run-time error results if the real is outside of permissible integer values. | ✓; same |
| ABR | 227 | E3 | | Absolute Value of Real. Take the absolute value of the real Tos. | ✓ |
| NGR | 228 | E4 | | Negate Real. Negate the Real Tos. | ✓ |
| DUP2 | 198 | C6 | | Copy Real. Duplicate two words on Tos. | ✓ |
| ADR | 192 | C0 | | Add Reals. Add Tos and Tos-1. | ✓ |
| SBR | 193 | C1 | | Subtract Reals. Subtract Tos from Tos-1. | ✓ |
| MPR | 194 | C2 | | Multiply Reals. Multiply Tos and Tos-1. | ✓ |
| DVR | 195 | C3 | | Divide Reals. Divide Tos-1 by Tos. | ✓ |
| EQUREAL | 205 | CD | | Compare Real =. Compare Tos-1 to Tos and push true or false. | ✓ |
| LEQREAL | 206 | CE | | Compare Real <=. Compare Tos-1 to Tos and push true or false. | ✓ |
| GEQREAL | 207 | CF | | Compare Real >=. Compare Tos-1 to Tos and push true or false. | ✓; settles C-3's title slip ("Compare Real >.", section 2.13) |
| ADJ | 199 | C7 | UB | Adjust Set. The set Tos is forced to occupy UB words, either by expansion (putting zeroes "between" Tos and Tos-1) or compression (chopping of high words of set), and its length word is discarded. | ✓ |
| SRS | 188 | BC | | Build Subrange Set. The integers Tos and Tos-1 are checked to insure that 0<=Tos<=4079 and 0<=Tos-1<=4079, otherwise a run-time error occurs. The set [Tos-1..Tos] is pushed. (The set [ ] is pushed if Tos-1 > Tos.) | ✓ |
| INN | 218 | DA | | Set Membership. See if integer Tos-1 is in set Tos, pushing true or false. | ✓ |
| UNI | 219 | DB | | Set Union. The union of sets Tos and Tos-1 is pushed. (Tos or Tos-1.) | ✓ |
| INT | 220 | DC | | Set Intersection. The intersection of sets Tos and Tos-1 is pushed. (Tos and Tos-1.) | ✓ |
| DIF | 221 | DD | | Set Difference. The difference of sets Tos-1 and Tos is pushed. (Tos-1 and not Tos.) | ✓ |
| EQUPWR | 182 | B6 | | Set Compare =. | ✓ |
| LEQPWR | 183 | B7 | | Set Compare <= (Subset of). | ✓ |
| GEQPWR | 184 | B8 | | Set Compare >= (Superset of). | ✓ |
| EQUBYT | 185 | B9 | B | Byte Array Compare =. | ✓ |
| LEQBYT | 186 | BA | B | Byte Array Compare <=. | ✓ |
| GEQBYT | 187 | BB | B | Byte Array Compare >=. | ✓ |

`ADJ` in C-1 says "its length word is discarded": after `ADJ` the set is a bare block of UB words
with no length word on top. This matches C-3's `sp := sp + len - UB + 1`, which drops the length
word.

### 6.6 Jumps (C-10)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| UJP | 138 | 8A | SB | Unconditional Jump. Jump to location with offset SB from the current location. | ✓ |
| FJP | 212 | D4 | SB | False Jump. Jump to location with offset SB from the current location if Tos is false. | ✓ |
| EFJ | 210 | D2 | SB | Equal False Jump. Jump to location with offset SB from the current location if integer Tos <> Tos-1. | ✓ |
| NFJ | 211 | D3 | SB | Not Equal False Jump. Jump to location with offset SB from the current location if integer Tos = Tos-1. | ✓ |
| UJPL | 139 | 8B | W | Unconditional Long Jump. Jump unconditionally to location with offset W from the current location. | ✓ |
| FJPL | 213 | D5 | W | False Long Jump. Jump to location with offset W from the current location if Tos is false. | ✓ |
| XJP | 214 | D6 | B | Case Jump. (below) | ✓ |

`XJP`, as printed: "The word with offset B in the constant word block is W1, the minimum index of
the Case Table. The word with offset B+1 in the constant word block is W2, the maximum index of
the Case Table. The Case Table starts in location with offset B+2 in the constant word block and
has a length of W2-W1+1 words. Tos is an index. If tos is in the range of W1..W2, then fetch the
contents of the location with tos index in the Case Table and jump to the location with this
offset from the current location."

- C-3 writes the "constant word block" as `segb + B`, so the constant word block is addressed
  from the start of the code segment. `LCA` and `LDC` use the same block.
- ⚠ "the current location" is not defined further here either (section 2.17): which byte `pc`
  holds when the offset is added is still to be settled from compiled code.

### 6.7 Procedure and function calls and returns (C-11, C-12)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| CPL | 144 | 90 | UB | Call Local Procedure. Call procedure UB, which is an immediate child of the currently executing procedure and in the same segment. Static link of MSCW is set to old MP. | ✓ |
| CPG | 145 | 91 | UB | Call Global Procedure. Call procedure UB, which is at the outer most lex level and in the same segment. The static link of the MSCW is set to base. | ✓ |
| CPI | 146 | 92 | DB,UB | Call Intermediate Procedure. Call procedure UB, which is at lex level DB less than the currently executing procedure and in the same segment. Use that activation record's static link as the static link of the new MSCW. | ✓ |
| CXL | 147 | 93 | UB1,UB2 | Call Local External Procedure. Call procedure UB2 which is an immediate child of the currently executing procedure and in the segment UB1. | ✓; settles C-3's `UB` = **UB2** (section 2.19) |
| CXG | 148 | 94 | UB1,UB2 | Call Global External Procedure. Call procedure UB2 which is at the outer most lex level and in the segment UB1. | ✓; same |
| CXI | 149 | 95 | UB1,DB,UB2 | Call Intermediate External Procedure. Call procedure UB2 which is at lex level DB less than the currently executing procedure, and in the segment UB1. | ✓; same |
| CPF | 151 | 97 | | Call Formal Procedure. Tos contains segment number and procedure number and Tos-1 contains static link for the called procedure. | ✓ |
| RPU | 150 | 96 | B | Return From User Procedure. Static link is discarded, MP is reset from MSDYN, IPC is also reset from MSIPC. If segment number is not zero, segment pointer is set from segment dictionary. Stack pointer is decremented by B. | ✓ |
| LSL | 153 | 99 | DB | Load Static Link Onto Stack. DB indicates the number of static links to traverse to get the static link to load. | ✓ |

- `CPI`: "Use that activation record's static link" is the wording the IV.0 book later repeats
  (section 2.18). The activation record "at lex level DB less than the currently executing
  procedure" is found from the **caller**, so the walk starts from the caller's frame, as the ⚠
  in section 2.18 reads it.
- `RPU`: "Stack pointer is decremented by B" means the stack **shrinks** by B words. The stack
  grows toward low memory, so in C-3 that is `sp := sp + B + mscw_sz`. Not a contradiction.
- `RPU` says nothing about `segrefs` (the ⚠ in section 2.19 stays open).

### 6.8 System control (C-12)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| SIGNAL | 222 | DE | | Signal. Tos is a semaphore address. Signal this semaphore. | ✓ |
| WAIT | 223 | DF | | Wait on Semaphore. Tos is a semaphore address. Wait on this semaphore. | ✓ |
| LPR | 157 | 9D | | Load Processor Register. Tos is a register #. (If it is positive it is one of the TIB registers. If not -1 is the current task pointer, -2 is the segment dictionary pointer and -3 is the ready queue pointer.) Load contents of this register on top of stack. | ✓ |
| SPR | 209 | D1 | | Store Processor Register. Tos-1 is a register number (same definition as LPR). Store Tos in this register. | ✓; agrees with the reading of C-3's garbled `SPR` (section 2.22) |

### 6.9 Debugger and miscellaneous (C-13)

| Mnemonic | Dec | Hex | Par | Description (C-1) | vs C-3 |
|---|---|---|---|---|---|
| BPT | 158 | 9E | | Break Point. | ✓ |
| RBP | 159 | 9F | B | Return From Breakpoint. This acts like an RPU operator. The stack pointer is decremented by B. | **not in C-3**; ⚠ same opcode as `BNOT` (C-6), see section 2.22 |
| NOP | 156 | 9C | | No Operation. | ✓ |
| SWAP | 189 | BD | | Swap word Top-of-Stack with word Top-of-Stack-1. | ✓ |

### 6.10 C-1 against C-3: the result

Checked by script over the two transcriptions:

- **All 100 operators of Table C-3 are in Table C-1 with the same opcode and parameters.** No
  opcode or parameter differs.
- C-1 has **one operator more, `RBP` (159)**, which collides with `BNOT` (section 2.22).
- C-1's prose settles four of C-3's ⚠ points: `STM`'s operand order (6.2), `LDP`'s dereference
  (6.3), `GEQREAL`'s title (6.5), and `UB2` as the procedure number of `CXL`/`CXG`/`CXI` (6.7).
  It agrees with the reading of `SPR` (6.8).
- **Opcode coverage:** 168 of the 256 byte values are assigned (the short forms `SLDC`, `SLDL`,
  `SLDO` and `SIND` take 72 of them). **Unassigned: `40`..`77`, `A8`..`AF`, `E8`..`FF`** (88
  values). Several of these were given operators in the later IV.0 p-machine (for example
  `70`..`77` short external calls, `E8`..`EC` string operators; the TI *Internal Architecture*,
  appendix A), so a III.0 code file that uses one was compiled for another p-machine.
- ⚠ What the MicroEngine does when it fetches an unassigned opcode is not stated in C-1 or C-3.
  The core should stop and report it rather than guess; the real machine can tell us later if
  it ever matters.

---

## 7. Readings settled from real code and the Pascal-100 manual

The ⚠ notes above give a reading for each unclear point. This section records the evidence that
settles each reading, or says that the point stays open. The evidence comes from three sources:

- the owner's Digicomp distribution disk "Pascal-100 System III.0 F1-2.0" (7/14/81, disk 033 of
  the owner's collection). Its 19 code files were decoded with the opcode table of section 6.
  Every opcode in them is an assigned opcode, except the `FF` bytes listed in 7.10.
- the Pascal-100 manual, appendix F 3.7.2 (the MicroEngine bootstrap flowchart).
- WD Technical Note M0015 (Sept 1981), reprinted in the Pascal-100 manual, appendix H. It gives a
  stack picture before and after each operator.

The `pascal100` P-machine core implements the readings in this section.

### 7.1 `pc` is a byte offset from `segb`

Flowchart step 20 (Pascal-100 manual, F 3.7.2): "EXECUTE FIRST INS FROM MEMORY LOC POINTED BY
((SGP) + (IPC)/2)". The instruction byte at `pc` is byte `pc mod 2` of word `segb + pc div 2`.
This agrees with `msipc`, "byte index in return code seg" (C-34).

### 7.2 The procedure dictionary and the data size (sections 2.18, 2.19)

The real code shows a layout that differs from the C-3 text by one word, and has one more level:

- The word at `segb + segb^.i` holds the segment number in its low byte and the number of
  procedures in its high byte. For example, CALC.CODE segment 1 has `0E01`: segment 1, 14
  procedures.
- The entry for procedure *n* is the word at `segb + segb^.i - n`. It is not at `- n - 1`, as C-3
  prints it. In CALC.CODE, procedure 1 calls `CPG 14`, and the segment has 14 procedures.
  C-3's `- n - 1` would read the word below the dictionary for procedure 14.
- The entry is a **word offset from `segb`** to the procedure's header word. It is not a `pc`.
- The header word is the procedure's **data size** (`data_sz` in `createmscw`): the number of
  local words, without the parameters. The first instruction is the next word, at byte offset
  `2 * entry + 2`.

Example, CALC.CODE procedure 5: entry `001E`, header `0007`. The code reads a parameter with
`SLDL8` and stores a real result at local offsets 9 and 10, then ends with `RPU 8`. Locals 1 to
7, one parameter at 8, and a two-word function result above it agree with that data size. In
all 14 procedures of the segment, `RPU`'s `B` is the data size plus the parameter words.

The core reads `data_sz` from the header word. A call sets `pc := 2 * entry + 2`, where `entry`
is the dictionary word for the procedure.

### 7.3 Jump offsets count from the next instruction (sections 2.17, 6.6)

The offset of `UJP`, `FJP`, `EFJ`, `NFJ`, `UJPL`, `FJPL` and `XJP` is added to the address of
the instruction after the jump, that is, after all its parameters.

- CALC.CODE procedure 5: `FJP 29` at byte 83 skips an `if` body that ends with `STM 2` at
  byte 112. From the next instruction, the jump goes to byte 114, after the body. From the
  opcode, the jump would go to byte 112 and run the `STM` on the wrong stack.
- SYSTEM.PASCAL segment 4 (PRINTERROR): `XJP 334` has the case range 1 to 15. From the next
  instruction, each of the 15 offsets goes to the start of a message (`LLA 10; LCA ...`). From the
  opcode, each offset goes to the `UJPL` that ends the case before it.

### 7.4 The word order of a real (section 2.12)

On the stack, the word on top holds the sign, the exponent and the high mantissa bits. The word
under it holds the low mantissa bits. In memory, a real variable at word `a` has the high word at
`a` and the low word at `a + 1`. The two words together are IEEE single precision.

Evidence: `LDC` constants in the code files, read with the high word first: `3F80 0000` = 1.0,
`4000 0000` = 2.0, `3F00 0000` = 0.5, `3FC9 0FDB` = pi/2, `4049 0FDB` = pi, `402D F855` = e,
`3EDE 5BD9` = log10 e. `LDC` puts the word at offset `B` on top of the stack (section 2.7).

### 7.5 `STM` operand order (sections 2.7, 6.2)

The block of `UB` words is on top of the stack. The destination address is under it, at
`sp + UB`, as C-1 says. Real code pushes the address first: `LLA 1; SLDC1; FLT; STM 2`. M0015
draws `STM` as `[word-block,addr] : []`, which puts the address on top. That picture is wrong.

### 7.6 The static link walk of `CPI` and `CXI` (sections 2.18, 2.19)

The walk starts at the **caller's** `mp` and follows `DB` static links. The result is the static
link of the new mark stack. Evidence, CALC.CODE: procedures 12 and 13 are both declared in
procedure 11, because procedure 11 calls each with `CPL`. Procedure 13 calls procedure 12 with
`CPI 1,12`. One link from procedure 13's frame is procedure 11's frame, which is the correct
static link for procedure 12. `LOD` uses the same count: the `DB` of the `LOD`s in the nested
procedures 9, 10, 11 and 12 is 3, 4, 5 and 6.

### 7.7 `WAIT` and `SIGNAL` pop the semaphore address (section 2.21)

M0015 draws both as `[addr] : []`. In SYSTEM.PASCAL, no `WAIT` is followed by code that removes
the address. For example, segment 3 has `LDA 2,70; WAIT; CPG 60` and `LLA 1; WAIT; RPU 2`.
`WAIT` decrements the count when it does not block (the ⚠ in section 2.21).

### 7.8 `SPR` and `LPR` (section 2.22)

`SPR` takes the register number from under the value, as C-1 says. Real code:
`SLDC6; LSL 0; SPR` (`bp := mp`), `SLDC1; LDCB 192; SPR` (TIB word 1, the priority),
`SLDC4; SLDC5; LPR; SPR` (`sp := mp`). Register numbers that real code uses: -2, -1, 1, 4, 5, 6,
11 (`sibs`, the III.0 TIB layout of C-34).

Because `SPR 4` must leave `sp` at the stored value, `SPR` works in this order:

1. Take the value and the register number, then pop both.
2. Save the live registers in the TIB (`updatetib`).
3. Store the value. For -1, switch task (label 5).
4. For a positive number, load the live registers from the TIB again.

### 7.9 Run-time error numbers

The PRINTERROR segment of SYSTEM.PASCAL maps the numbers to messages with an `XJP` (7.3):

| No. | Message | No. | Message |
|---|---|---|---|
| 1 | Value range error | 9 | System IO error |
| 2 | No proc in seg-table | 10 | (user I/O error, with the I/O result) |
| 3 | Exit from uncalled proc | 11 | Unimplemented instruction |
| 4 | Stack overflow | 12 | Floating point error |
| 5 | Integer overflow | 13 | String overflow |
| 6 | Divide by zero | 14 | Programmed HALT |
| 7 | NIL pointer reference | 15 | Programmed break-point |
| 8 | Program interrupted by user | other | Unknown run-time error |

This list differs from the Pascal-100 manual, B.1, which gives 14 "Halt, Breakpoint" and 15
"Bad Block". The core uses the disk's numbers: `range_error` = 1, `stack_overflow` = 4,
`div_by_zero_error` = 6, floating point = 12, `Breakpoint` = 15. No message is for `MODI` with a
divisor of 0 or less: the real machine raises 1, `Value range error`, for `7 mod 0` and for
`7 mod -1` (7.12).

### 7.10 `FF` bytes in the code files

Decoding found `FF` in two places. `FF` is unassigned (section 6.10).

- The segment-0 entries of user programs (6 to 8 words). These are not code that runs.
- SYSTEM.PASCAL segment 0: 8 words, one procedure with data size 467 (the OS globals). Its
  code is `CXG 3,1` (the OS main program) and then `FF` bytes. So segment 0 calls out once and
  is never returned to: a return would find `msseg = 0` (2.19 note) and the `FF` bytes.
- SYSTEM.PASCAL segment 3 (CSPCODE), procedure 39: the whole body is `FF`, then `RPU 0`. If that
  procedure runs, the core stops and reports the opcode (section 6.10). Its purpose is open.

### 7.11 Still open: check on the real machine

Real code on disk 033 does not settle these points. The core uses the reading in the ⚠ note until
the real machine shows otherwise.

| Point | Reading the core uses | Why the disk does not settle it | Check on the real machine |
|---|---|---|---|
| Opcode `9F`: `BNOT` or `RBP` (2.22) | `BNOT` (M0015 lists it for III.0) | No code file uses `9F`. The compiler makes `LNOT` for a Boolean `not`. | Run `PMACHINE` with `9F` on a known word, and print the result |
| `BPT` error number (2.22) | 15 | The OS message list says 15, B.1 says 14 | Run `PMACHINE(158)` |
| Which byte holds `msseg` (1) | the low byte (Appendix I) | Code does not read the mark stack | Read a mark stack with `LPR 5` in a `PMACHINE` program |
| `CPF` and `msseg`, `RPU` and `segrefs` (2.19) | the notes of section 2.19 | No code file uses `CPF`, `LDE`, `LAE` or `STE` | Only if a program that uses them fails |
| `SPR -1`, store the current task (2.22) | Save the live registers in the old TIB, set `ctp`, then load the registers from the new TIB. C-3 prints `goto 5`, which would take the next task from the ready queue and ignore the stored value | No code file stores register -1 (7.8) | Only if a program that does it fails |
| CSPCODE procedure 39 (7.10) | stop and report | Purpose unknown | Look for it when the boot runs (Stage 5) |

### 7.12 Settled on the real machine

| Point | Reading | How it was settled |
|---|---|---|
| Real errors (2.13) | Overflow in ADR and MPR, DVR by 0, and TNC and RND outside the integer range raise 12. **Underflow does not raise an error, against C-3 and the manual:** a result below the smallest normal number is +0, for either sign, in MPR, DVR and SBR | 2026-11-04: programs RERR and RERR2 run on the real Pascal-100, each error case as the first error after a boot. RERR2 printed the bits of each result. Not tested: ADR underflow, and a result that rounds to the smallest normal number |
| Stack overflow test in `createmscw` (2.18) | The frame must fit ONCE: overflow when `sp - mscw_sz - data_sz < splow`. The printed second test, `data_sz + mscw_sz > sp - splow`, is the guard against a 16-bit wrap of the subtraction, and only works with the OLD `sp`. Read with the new `sp`, a call needs room for its frame twice over | 2026-11-03, Stage 5. With the twice-over reading, the Filer on the user's boot disk stopped with `*STK OFLOW*` at `V` then `L #5:` (a 260-word frame, 516 words free). The real Pascal-100, same boot disk (STARTUPA) and disk 081 in B, lists the directory: `10/10 files<listed/in-dir>, 284 blocks used, 850 unused`. The simulator with the once reading prints the same |
| `MODI` error number (2.11) | **1**, `Value range error` (not 6), for a divisor of 0 and for a negative divisor | 2026-11-03, Stage 5: program T711 compiled in the simulator and run on the real Pascal-100 from the same disk image. The real box printed `Value range error` for `7 mod -1`; the simulator now raises 1. After the space, both machines print `Unknown run-time error` again and again (III.0's own behavior on this system). `7 mod 0`, run as the first error after a boot: `Value range error` on both |
| `MODI` with a negative dividend (2.11) | `y - (abs(x) mod y)`: a multiple of `y` gives `y` | 2026-11-03, Stage 5: program T711 compiled in the simulator and run on the real Pascal-100 from the same disk image. The real box printed `-7 MOD 3 = 2   -6 MOD 3 = 3` |
| Rounding of real arithmetic (2.13) | IEEE round to nearest | 2026-11-03, Stage 5: program T711 compiled in the simulator and run on the real Pascal-100 from the same disk image. The real box printed `1/3 WORDS: -21845 16042` (= `3EAA AAAB`) and `2/3 WORDS: -21845 16170` |
| String compare `LEQBYT`/`GEQBYT` (2.16) | lexicographic, TOS-1 against TOS | 2026-11-03, Stage 5: program T711 compiled in the simulator and run on the real Pascal-100 from the same disk image. The real box printed `AZ <= BA: 1   AZ >= BA: 0` |
