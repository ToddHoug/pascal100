# THE PASCAL-100 BOOT TEST: UCSD Pascal III.0 boots on the Pascal-100 machine
# (tests/media/pascal100/machine-pascal100.toml), and the
# Filer reads the disk.
#
# THE DISK is tests/media/pascal100/pascal-dd.dsk, a system disk built for this machine by manual ch.
# V D (FBOOT, BIOS and QGEN, made for the machine's own I/O): the distribution disks do not boot
# as they are. To test another system disk, set PASCAL100_BOOT_DISK to its absolute path (.dsk or
# .imd). A path that is relative or that does not exist fails the test.
#
# The whole boot runs here, as manual ch. V B gives it:
#   1. the Disk 1 EPROM (routine 4) reads track 0, and the disk's FBOOT reads SBQB and the
#      Z80 BIOS to 280H;
#   2. SBQB reads the Pascal boot, loads the Map, writes the init vector at 1F8D0 and starts
#      the MicroEngine;
#   3. the Pascal boot, on the MicroEngine, loads SYSTEM.Z80BIOS, SYSTEM.Z80CODE and
#      SYSTEM.PASCAL through the Z80 BIOS;
#   4. the operating system prints the III.0 sign-on and the command line.
# Then the Filer, with a second copy of the disk in drive B: V lists the volumes, L #5: lists
# the disk in drive B (the large frame that the old stack test refused), L lists the boot volume. Every disk read after step 2
# is a MicroEngine request that the Z80 serves, so this exercises the processor switch, the
# Map and the shared memory hundreds of times.
#
# The checks use only text that every III.0 system disk prints, not the user's volume name.
# The guest can write, so the test boots a copy.
#
# Expects: -DSIM=<pascal100> -DSRC=<source dir> -DBIN=<binary dir>

cmake_minimum_required(VERSION 3.20)

# tests/media/pascal100/pascal-dd.dsk by default (the same image as the example's). PASCAL100_BOOT_DISK
# tests another system disk instead.
set(disk "$ENV{PASCAL100_BOOT_DISK}")
if(disk STREQUAL "")
  set(disk "${SRC}/tests/media/pascal100/pascal-dd.dsk")
endif()
# A disk that is set but not there is an error, not a skip: a typo must not look like a pass.
# ctest runs this script in the build directory, not where the variable was set, so a
# relative path would resolve against the wrong place.
if(NOT IS_ABSOLUTE "${disk}")
  message(FATAL_ERROR "pascal100-boot: PASCAL100_BOOT_DISK=${disk}: give an absolute path")
endif()
if(NOT EXISTS "${disk}" OR IS_DIRECTORY "${disk}")
  message(FATAL_ERROR "pascal100-boot: PASCAL100_BOOT_DISK=${disk}: no such file")
endif()

set(work "${BIN}/pascal100-boot-work")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
get_filename_component(ext "${disk}" LAST_EXT)
string(TOLOWER "${ext}" ext)
if(NOT ext STREQUAL ".imd")
  set(ext ".dsk")
endif()
configure_file("${disk}" "${work}/boot${ext}" COPYONLY)
# A second copy in drive B: Filer L of a second volume needs a large frame (reference 7.12).
configure_file("${disk}" "${work}/second${ext}" COPYONLY)

set(session "")
function(call id tool args)
  set(session "${session}{\"jsonrpc\":\"2.0\",\"id\":${id},\"method\":\"tools/call\",\"params\":{\"name\":\"${tool}\",\"arguments\":${args}}}\n" PARENT_SCOPE)
endfunction()

set(session "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}\n{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}\n")
call(2 monitor "{\"command\":\"MOUNT fd0:drive0 \\\"${work}/boot${ext}\\\"\"}")
call(9 monitor "{\"command\":\"MOUNT fd0:drive1 \\\"${work}/second${ext}\\\"\"}")
# MCP does not run the machine file's startup, so start the boot EPROM at 0000 here.
call(3 run "{\"from\":0,\"until\":\"Command: E(dit\",\"timeout_ms\":60000}")
call(4 run "{\"until\":\"]\",\"timeout_ms\":5000}")
call(5 run "{\"input\":\"F\",\"until\":\"Q(uit\",\"timeout_ms\":30000}")
call(6 run "{\"input\":\"V\",\"until\":\"Q(uit\",\"timeout_ms\":30000}")
call(7 run "{\"input\":\"L\",\"until\":\"vol ?\",\"timeout_ms\":10000}")
call(10 run "{\"input\":\"#5:\\r\",\"until\":\"SYSTEM.FILER\",\"timeout_ms\":30000}")
call(11 run "{\"input\":\" \",\"until\":\"Q(uit\",\"timeout_ms\":30000}")
call(12 run "{\"input\":\"L\",\"until\":\"vol ?\",\"timeout_ms\":10000}")
call(8 run "{\"input\":\"*\\r\",\"until\":\"SYSTEM.FILER\",\"timeout_ms\":30000}")
file(WRITE "${work}/in.jsonl" "${session}")

execute_process(
  COMMAND           "${SIM}" "${SRC}/tests/media/pascal100/machine-pascal100.toml" --mcp
  WORKING_DIRECTORY "${work}"
  INPUT_FILE        "${work}/in.jsonl"
  OUTPUT_VARIABLE   out
  ERROR_VARIABLE    err
  RESULT_VARIABLE   rc
  TIMEOUT           200
)

function(fail why)
  message(FATAL_ERROR "pascal100-boot: ${why}\n--- stdout ---\n${out}\n--- stderr ---\n${err}")
endfunction()

if(NOT rc EQUAL 0)
  fail("`pascal100 machine-pascal100.toml --mcp` exited ${rc}, expected 0")
endif()

function(reply id var)
  string(REPLACE "\r" "" out_lf "${out}")
  string(REGEX MATCH "(^|\n){[^\n]*\"id\":${id}[,}][^\n]*" line "${out_lf}")
  set(${var} "${line}" PARENT_SCOPE)
endfunction()

reply(2 r)
if(NOT r MATCHES "mounted")
  fail("MOUNT of the boot disk failed")
endif()
reply(9 r)
if(NOT r MATCHES "mounted")
  fail("MOUNT of the second disk failed")
endif()
foreach(id 3 4 5 6 7 10 11 12 8)
  reply(${id} r)
  if(NOT r MATCHES "\"stopped\":\"match\"")
    fail("run ${id} did not reach its text")
  endif()
endforeach()

# Steps 1-4: the sign-on of the operating system, printed by the MicroEngine through the Z80.
reply(3 r)
if(NOT r MATCHES "U\\.C\\.S\\.D\\.  Pascal  System  III\\.0")
  fail("the boot did not print the III.0 sign-on")
endif()
# The Filer's volume list and the directory of the boot volume.
reply(6 r)
if(NOT r MATCHES "Volumes on-line:" OR NOT r MATCHES "CONSOLE:" OR NOT r MATCHES "Prefix is")
  fail("Filer V did not list the volumes")
endif()
# Filer L of the volume in drive B, after V: the frame of 260 words that the old stack test
# refused with *STK OFLOW* (reference 7.12).
reply(6 r)
if(NOT r MATCHES "  5 \\* ")
  fail("Filer V did not list the second disk as volume 5")
endif()
reply(10 r)
if(r MATCHES "STK OFLOW" OR NOT r MATCHES "SYSTEM\\.FILER")
  fail("Filer L #5: did not list the second disk")
endif()
reply(8 r)
if(NOT r MATCHES "SYSTEM\\.FILER")
  fail("Filer L did not list the boot volume")
endif()
