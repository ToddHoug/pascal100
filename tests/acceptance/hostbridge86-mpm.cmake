# THE HOST BRIDGE LOCK ON MP/M 8-16: two users, one board.
#
# The board has one transfer stream. Under MP/M-86, R.CMD, W.CMD and HDIR.CMD share the mutual
# exclusion queue MXhostbr (cpm/hostbridge/R.A86, LOCK): a second user waits until the first is
# done. This test boots a scratch copy of tests/media/compupro/mpm816 (the mpm8-16 example's
# machine and disks), starts a long R on the console and another on tty1 at the same moment,
# and proves that:
#
#   - one of the two waited ("another user has the host bridge. Waiting..."),
#   - both copies finished, and
#   - each file comes back out through W byte for byte, so the two streams did not mix.
#
# The R.CMD, W.CMD and HDIR.CMD on that M20 are the committed ones. When they are rebuilt, put
# the new ones on this M20 too, as on the example's (A:, SET [SYS,RO], SYNC).
#
# Expects: -DSIM=<pascal100> -DSRC=<source dir> -DBIN=<binary dir>

string(ASCII 13 CR)
set(work "${BIN}/hostbridge86-mpm")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
foreach(f mpm816-console.toml mpm816-boot.dsk m20.dsk)
  file(COPY "${SRC}/tests/media/compupro/mpm816/${f}" DESTINATION "${work}")
endforeach()

# Two host files, each 1024 CP/M records (131072 bytes: 2048 lines of 64 bytes), so that R
# pads nothing and W ... B gives back the same bytes. The two differ in their first character,
# so a mixed stream shows. file(WRITE) on Windows turns LF into CRLF itself (hostbridge.cmake).
if(WIN32)
  set(eol "\n")
else()
  set(eol "${CR}\n")
endif()
foreach(tag A B)
  string(REPEAT "${tag}123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" 1 line)
  string(SUBSTRING "${line}" 0 62 line)
  string(REPEAT "${line}${eol}" 2048 body)
  file(WRITE "${work}/BIG${tag}.TXT" "${body}")
  file(SIZE "${work}/BIG${tag}.TXT" s)
  if(NOT s EQUAL 131072)
    message(FATAL_ERROR "hostbridge86-mpm: BIG${tag}.TXT is ${s} bytes, want 131072")
  endif()
endforeach()

# ---- the session ----
set(lines [=[{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"hostbridge86-mpm","version":"1"}}}
{"jsonrpc":"2.0","method":"notifications/initialized"}]=])
set(id 2)
function(req json)
  set(lines "${lines}\n{\"jsonrpc\":\"2.0\",\"id\":${id},\"method\":\"tools/call\",\"params\":${json}}" PARENT_SCOPE)
  math(EXPR n "${id} + 1")
  set(id ${n} PARENT_SCOPE)
endfunction()

req([=[{"name":"run","arguments":{"from":0,"until":"0A>","timeout_ms":120000}}]=])
req([=[{"name":"run","arguments":{"unit":"if4:user3","input":"\r","until":"0A>","timeout_ms":30000}}]=])
# THE TWO AT ONCE. `send` types on the console without running the guest; the next `run`
# types on tty1 and runs the guest, so both commands start together. Whichever gets the board
# first, the other waits for it. tty1's R ends after both are done or after its own.
req([=[{"name":"send","arguments":{"text":"R BIGA.TXT\r"}}]=])
req([=[{"name":"run","arguments":{"unit":"if4:user3","input":"R BIGB.TXT\r","until":"bytes","timeout_ms":180000}}]=])
# The console's output so far, then a CR that gives a new prompt once its R is done.
req([=[{"name":"recv","arguments":{}}]=])
req([=[{"name":"run","arguments":{"input":"\r","until":"0A>","timeout_ms":180000}}]=])
req([=[{"name":"run","arguments":{"input":"W BIGA.TXT BACKA.TXT B\r","until":"0A>","timeout_ms":120000}}]=])
req([=[{"name":"run","arguments":{"input":"W BIGB.TXT BACKB.TXT B\r","until":"0A>","timeout_ms":120000}}]=])
# SYNC prints its prompt before MP/M writes the cache (examples/mpm8-16/README.md), so let the
# machine run a moment after it. Nothing reads the disk after this run, but a test leaves the
# image as a person would.
req([=[{"name":"run","arguments":{"input":"SYNC\r","until":"0A>","timeout_ms":60000}}]=])
req([=[{"name":"run","arguments":{"until":"@@never@@","timeout_ms":1000}}]=])
file(WRITE "${work}/session.jsonl" "${lines}\n")

execute_process(
  COMMAND           "${SIM}" mpm816-console.toml --mcp
  WORKING_DIRECTORY "${work}"
  INPUT_FILE        "${work}/session.jsonl"
  OUTPUT_VARIABLE   out
  ERROR_VARIABLE    out
  TIMEOUT           600
)
file(WRITE "${work}/guest.log" "${out}")

function(want text why)
  string(FIND "${out}" "${text}" hit)
  if(hit LESS 0)
    message(FATAL_ERROR "hostbridge86-mpm: ${why}\n  '${text}' never reached a terminal.\n"
                        "  transcript: ${work}/guest.log")
  endif()
endfunction()
want("R: BIGA.TXT -> BIGA.TXT" "the console's R did not finish")
want("R: BIGB.TXT -> BIGB.TXT" "tty1's R did not finish")
want("1024 records, 131072 bytes" "an R did not copy all of its file")
want("another user has the host bridge. Waiting..." "neither R waited for the other: no lock")
want("W: BIGA.TXT -> BACKA.TXT" "W did not copy BIGA.TXT back")
want("W: BIGB.TXT -> BACKB.TXT" "W did not copy BIGB.TXT back")
foreach(tag A B)
  file(SHA1 "${work}/BIG${tag}.TXT" want)
  file(SHA1 "${work}/BACK${tag}.TXT" got)
  if(NOT want STREQUAL got)
    message(FATAL_ERROR "hostbridge86-mpm: BIG${tag}.TXT did not survive the round trip: the "
                        "two transfers mixed.\n  transcript: ${work}/guest.log")
  endif()
endforeach()
message(STATUS "hostbridge86-mpm: passed")
