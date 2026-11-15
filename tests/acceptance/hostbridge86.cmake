# THE 8086 HOST BRIDGE UTILITIES: R.CMD, W.CMD and HDIR.CMD on CompuPro CP/M 8-16.
#
# The same two modes as hostbridge.cmake (the 8080 R.COM, W.COM and HDIR.COM), and the same
# files moved, so a fault in either version shows up the same way:
#
#   MODE=fast    PIP the committed .H86 in through the console, GENCMD each into its .CMD
#                (as LOAD makes a .COM from a .HEX), and move files. This tests the card and
#                the committed .CMD.
#
#   MODE=build   PIP the committed .A86 in, ASM86 and GENCMD each, and prove that the .CMD,
#                .H86 and .LST that come out are byte for byte what is committed beside them.
#
# The machine is tests/media/compupro/cpm816-hb.toml: the CPU 8085/88, an Interfacer 1, a
# Disk 1 and the host bridge, on a scratch copy of CPM816.DSK (CP/M 8-16 1.1R, which has ASM86
# and GENCMD). CP/M 8-16 runs a .CMD on the 8088, so every R, W and HDIR below is the 8086 one.
# There is no .COM of them on the disk for SW! to find instead.
#
# Expects: -DSIM=<pascal100> -DSRC=<source dir> -DBIN=<binary dir> [-DMODE=fast|build]

if(NOT DEFINED MODE)
  set(MODE fast)
endif()

string(ASCII 13 CR)
string(ASCII 26 SUB)
# file(WRITE) on Windows turns LF into CRLF itself (see hostbridge.cmake).
if(WIN32)
  set(CRLF "\n")
else()
  set(CRLF "${CR}\n")
endif()

set(work "${BIN}/hostbridge86-${MODE}")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}/host/SRC")
file(MAKE_DIRECTORY "${work}/host/out")   # lower case: the card folds every part of a path

# ---- the host side: the same files as hostbridge.cmake ----
file(WRITE "${work}/host/HELLO.TXT" "HELLO FROM THE HOST.${CRLF}LINE TWO.${CRLF}")
file(WRITE "${work}/host/SRC/DEEP.TXT" "IN A SUBDIRECTORY OF HOSTDIR.${CRLF}")
file(WRITE "${work}/host/MY-NOTES(2).TXT" "NOTES.${CRLF}")
file(COPY "${SRC}/tests/acceptance/hostbridge.bin" DESTINATION "${work}/host")
file(RENAME "${work}/host/hostbridge.bin" "${work}/host/BIN.DAT")   # every byte 00..FF

# ---- the session ----
# EACH COMMAND IS TYPED ONLY AFTER THE A> PROMPT IS BACK, over MCP (`run` with `until`). Typing
# ahead, as hostbridge.cmake does, does not work here: ASM86 checks the console for a key while
# it assembles, and every waiting character is a USER BREAK. A file fed to PIP is CP/M text, so
# it is CRLF, and a ^Z ends it. Each string goes into JSON, so it is escaped first.
string(ASCII 9 TAB)
function(json_escape in outvar)
  string(REPLACE "\\" "\\\\" t "${in}")
  string(REPLACE "\"" "\\\"" t "${t}")
  string(REPLACE "${CR}" "\\r" t "${t}")
  string(REPLACE "\n" "\\n" t "${t}")
  string(REPLACE "${TAB}" "\\t" t "${t}")
  string(REPLACE "${SUB}" "\\u001a" t "${t}")
  set(${outvar} "${t}" PARENT_SCOPE)
endfunction()
# Functions, not macros: a macro reads a backslash in its argument as an escape again, and the
# JSON is full of them. Each request is appended to the session file at once.
file(WRITE "${work}/session.jsonl"
  "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{},\"clientInfo\":{\"name\":\"hostbridge86\",\"version\":\"1\"}}}\n"
  "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}\n")
set_property(GLOBAL PROPERTY hb86_id 2)
function(req json)
  get_property(id GLOBAL PROPERTY hb86_id)
  file(APPEND "${work}/session.jsonl"
       "{\"jsonrpc\":\"2.0\",\"id\":${id},\"method\":\"tools/call\",\"params\":${json}}\n")
  math(EXPR id "${id} + 1")
  set_property(GLOBAL PROPERTY hb86_id ${id})
endfunction()
function(mon line)
  json_escape("${line}" e)
  req("{\"name\":\"monitor\",\"arguments\":{\"command\":\"${e}\"}}")
endfunction()
function(typed text)
  json_escape("${text}" e)
  req("{\"name\":\"run\",\"arguments\":{\"input\":\"${e}\",\"until\":\"A>\",\"timeout_ms\":120000}}")
endfunction()
function(cmd line)
  typed("${line}${CR}")
endfunction()
function(pip file into)
  file(READ "${file}" body)
  string(REPLACE "\r" "" body "${body}")
  string(REPLACE "\n" "${CR}\n" body "${body}")
  typed("PIP ${into}=CON:${CR}${body}${SUB}")
endfunction()

configure_file("${SRC}/tests/media/compupro/CPM816.DSK" "${work}/work.dsk" COPYONLY)
mon("SET CONSOLE UPPER=OFF")            # the sources have lower case in strings
mon("SET hb0 HOSTDIR=${work}/host")
mon("MOUNT fd0:drive0 \"${work}/work.dsk\"")
req("{\"name\":\"run\",\"arguments\":{\"from\":0,\"until\":\"A>\",\"timeout_ms\":60000}}")

if(MODE STREQUAL build)
  foreach(u R W HDIR)
    pip("${SRC}/cpm/hostbridge/${u}.A86" "${u}.A86")
    cmd("ASM86 ${u}")                   # -> .H86, .LST, .SYM
    cmd("GENCMD ${u}")                  # -> .CMD
  endforeach()
else()
  foreach(u R W HDIR)
    cmd("ERA ${u}.CMD")                 # a guard: the program that runs is the one built here
    pip("${SRC}/cpm/hostbridge/${u}.H86" "${u}.H86")
    cmd("GENCMD ${u}")
    cmd("ERA ${u}.H86")
  endforeach()
endif()

# ---- does the card move files ----
cmd("HDIR")
cmd("R HELLO.TXT")
cmd("R BIN.DAT")
cmd("R SRC/DEEP.TXT")
cmd("W BIN.DAT BACKBIN.DAT")            # THE ROUND TRIP: every byte
cmd("W HELLO.TXT TRIM.TXT T")
cmd("W HELLO.TXT AUTO.TXT")             # .TXT: text, by extension
cmd("W HELLO.TXT PAD.TXT B")            # a switch wins over the extension
cmd("W BIN.DAT CUT.DAT T")              # ...both ways: cut at the 1AH
cmd("R HELLO.TXT MIX.Z80")
cmd("R BIN.DAT MIX.DAT")
cmd("W MIX.* OUT")                      # a wildcard into a host folder
cmd("W MIX.* NOPE")                     # ...which must exist
cmd("R ../SECRET.TXT")                  # the sandbox
cmd("R MY-NOTES(2).TXT")
# A WORD TOO LONG FOR ITS BUFFER IS REFUSED (altairsim #929). R copied each word with no limit,
# so a long CP/M name ran past its 20-byte buffer into the stack, and R saved a file under a
# name nobody typed. W stopped at its limit but left the rest of the word, which became the
# next word: here `XB` after 63 X's, read as the switch. Each word may now be 64 characters,
# and a longer one is refused with nothing written. A CP/M name of 64 is still cut to 8.3.
string(REPEAT "C" 64 fitcp)
string(REPEAT "B" 65 longcp)
string(REPEAT "H" 65 longhost)
string(REPEAT "X" 64 longw)
cmd("R HELLO.TXT ${fitcp}")             # the second word: 64 fits, and R stores CCCCCCCC
cmd("R HELLO.TXT ${longcp}")            # the second word: 65 > 64
cmd("R ${longhost}")                    # the first word: 65 > 64
cmd("W HELLO.TXT ${longw}B")            # the host name: 65 > 64
cmd("DIR BBBBBBBB.*")                   # the issue's check: R made no BBBBBBBB.xxx
cmd("HDIR *.TXT")

# The .CMD of each, and in build mode the .H86 and .LST too (text by extension: W stops at
# the ^Z that ends each), to compare with what is committed.
foreach(u R W HDIR)
  cmd("W ${u}.CMD")
endforeach()
if(MODE STREQUAL build)
  foreach(u R W HDIR)
    cmd("W ${u}.H86")
    cmd("W ${u}.LST")
  endforeach()
endif()

# ---- run the machine ----
# The leash is for a HUNG machine, not a slow runner: fast mode takes ~11 s on an M-series Mac,
# but 47 s and then 94.5 s on the Windows CI runner (2026-11-15), so 120 s was too close.
set(leash 240)
if(MODE STREQUAL build)
  set(leash 300)
endif()
execute_process(
  COMMAND           "${SIM}" "${SRC}/tests/media/compupro/cpm816-hb.toml" --mcp
  WORKING_DIRECTORY "${work}"
  INPUT_FILE        "${work}/session.jsonl"
  OUTPUT_VARIABLE   out
  ERROR_VARIABLE    out
  TIMEOUT           ${leash}
  RESULT_VARIABLE   rc
)
# MCP returns each result as a JSON string: turn the escaped line ends back into real ones, so
# the checks below read the console text.
string(REPLACE "\\r\\n" "\n" out "${out}")
string(REPLACE "\\n" "\n" out "${out}")
string(REPLACE "\\r" "" out "${out}")
file(WRITE "${work}/guest.log" "${out}")
if(rc STREQUAL "Process terminated due to timeout")
  message(FATAL_ERROR
    "hostbridge86(${MODE}): THE MACHINE HUNG -- killed after ${leash}s.\n"
    "  transcript: ${work}/guest.log")
endif()

function(want text why)
  string(FIND "${out}" "${text}" hit)
  if(hit LESS 0)
    message(FATAL_ERROR "hostbridge86(${MODE}): ${why}\n"
                        "  '${text}' never reached the terminal.\n"
                        "  full transcript: ${work}/guest.log")
  endif()
endfunction()
function(same a b why)
  if(NOT EXISTS "${b}")
    message(FATAL_ERROR "hostbridge86(${MODE}): ${why}\n  ${b} was not written.\n"
                        "  transcript: ${work}/guest.log")
  endif()
  file(READ "${a}" ha HEX)
  file(READ "${b}" hb HEX)
  if(NOT ha STREQUAL hb)
    message(FATAL_ERROR "hostbridge86(${MODE}): ${why}\n  ${a}\n  and ${b} differ.\n"
                        "  transcript: ${work}/guest.log")
  endif()
endfunction()
function(size_is file n why)
  file(SIZE "${file}" s)
  if(NOT s EQUAL n)
    message(FATAL_ERROR "hostbridge86(${MODE}): ${why} (${s} bytes, want ${n})")
  endif()
endfunction()

if(MODE STREQUAL build)
  want("NUMBER OF ERRORS:   0" "ASM86 did not assemble a source cleanly")
  string(REGEX MATCHALL "NUMBER OF ERRORS: +[0-9]+" counts "${out}")
  foreach(c ${counts})
    if(NOT c MATCHES ": +0$")
      message(FATAL_ERROR "hostbridge86(build): ASM86 reported errors: ${c}\n"
                          "  transcript: ${work}/guest.log")
    endif()
  endforeach()
endif()
want("RECORDS WRITTEN" "GENCMD never wrote a .CMD")
# Look only at what ran after the last GENCMD: PIP echoes each source, and the sources hold
# the message texts that the checks below look for.
string(FIND "${out}" "RECORDS WRITTEN" built REVERSE)
string(SUBSTRING "${out}" ${built} -1 out)
string(FIND "${out}" "no hostbridge board" hit)
if(NOT hit LESS 0)
  message(FATAL_ERROR "hostbridge86(${MODE}): IDENT failed: a utility could not find the card.\n"
                      "  transcript: ${work}/guest.log")
endif()

want("R: HELLO.TXT -> HELLO.TXT"   "R did not copy the text file in")
want("R: SRC/DEEP.TXT -> DEEP.TXT" "R did not reach into a subdirectory of hostdir")
want("R: MY-NOTES(2).TXT -> MY-NOTES.TXT" "R did not keep the legal `-` and cut the name at eight")
want("W: HELLO.TXT -> AUTO.TXT (text, by extension)" "W did not say the extension chose text")
want("outside the host directory"  "R was not stopped from escaping hostdir with `..`")
want("W: NOPE: no such file"      "W copied into a host folder that does not exist")

# A WORD TOO LONG FOR ITS BUFFER (altairsim #929): each of the three is refused, in order, and
# nothing was written on either side. The search ignores case: a CCP prints `NO FILE` or
# `No file`. Over MCP the A> prompt ends the reply before, so each command starts its own.
string(TOUPPER "${out}" ranuc)
set(at 0)
foreach(line
    "R HELLO.TXT ${fitcp}"              "R: HELLO.TXT -> CCCCCCCC"
    "R HELLO.TXT ${longcp}"             "R: a name is too long."
    "R ${longhost}"                     "R: a name is too long."
    "W HELLO.TXT ${longw}B"             "W: a name is too long."
    "DIR BBBBBBBB.*"                    "NO FILE")
  string(TOUPPER "${line}" line)
  string(SUBSTRING "${ranuc}" ${at} -1 rest)
  string(FIND "${rest}" "${line}" hit)
  if(hit LESS 0)
    message(FATAL_ERROR
      "hostbridge86(${MODE}): a word too long for its buffer: '${line}' is missing.\n"
      "  full transcript: ${work}/guest.log\n--- from there on ---\n${rest}")
  endif()
  string(LENGTH "${line}" len)
  math(EXPR at "${at} + ${hit} + ${len}")
endforeach()
file(GLOB cut "${work}/host/XXXX*")
if(cut)
  message(FATAL_ERROR "hostbridge86(${MODE}): W wrote a host file under a cut name: ${cut}")
endif()

same("${SRC}/tests/acceptance/hostbridge.bin" "${work}/host/BACKBIN.DAT"
     "THE BINARY DID NOT SURVIVE THE ROUND TRIP (256 bytes, 00..FF)")
file(SIZE "${work}/host/HELLO.TXT" hello)
size_is("${work}/host/TRIM.TXT" ${hello} "`W ... T` did not stop at the first ^Z")
size_is("${work}/host/AUTO.TXT" ${hello} "W did not use text mode for a .TXT file")
size_is("${work}/host/PAD.TXT"  128      "`W ... B` did not copy the whole record")
size_is("${work}/host/CUT.DAT"  26       "`W BIN.DAT CUT.DAT T` did not stop at the 1AH")
size_is("${work}/host/out/MIX.Z80" ${hello} "`W MIX.* OUT` did not write MIX.Z80 as text")
same("${SRC}/tests/acceptance/hostbridge.bin" "${work}/host/out/MIX.DAT"
     "`W MIX.* OUT` did not write MIX.DAT byte for byte")
if(EXISTS "${work}/host/NOPE")
  message(FATAL_ERROR "hostbridge86(${MODE}): `W MIX.* NOPE` created NOPE on the host")
endif()

# The committed build artifacts.
foreach(u R W HDIR)
  same("${SRC}/cpm/hostbridge/${u}.CMD" "${work}/host/${u}.CMD"
       "${u}.CMD differs from the committed one")
endforeach()
if(MODE STREQUAL build)
  foreach(u R W HDIR)
    same("${SRC}/cpm/hostbridge/${u}.H86" "${work}/host/${u}.H86" "${u}.A86 does not assemble to the committed ${u}.H86")
    same("${SRC}/cpm/hostbridge/${u}.LST" "${work}/host/${u}.LST" "${u}.A86 does not assemble to the committed ${u}.LST")
  endforeach()
endif()
message(STATUS "hostbridge86(${MODE}): passed")
