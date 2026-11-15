# Test ROM fixtures

ROM images the unit tests mount by FILE PATH. They are not built into pascal100 (2026-11-09):
upstream altairsim built them in as `builtin:` ROMs for its Altair machines, and the tests use
them because their properties are pinned by many checks.

- `DBL.HEX` -- the MITS 88-DCDD Disk Boot Loader (DESC has upstream's one-line description).
  256 bytes whose Intel HEX records place it at FF00; its first byte is 0x21 (LXI H). The
  memory-board tests rely on all three facts.
- ALTMON is in `../altmon/`, with the test machine that boots it.

Use `testRom("roms/DBL.HEX")` (tests/test.h) to get the absolute path.
