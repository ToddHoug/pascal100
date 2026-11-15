#!/bin/bash
# mcp-watch -- watch the console mirror of one of our MCP servers by its server name.
#
#   mcp-watch                  list the servers and their mirror ports
#   mcp-watch <server>         watch that server's mirror, e.g.  mcp-watch pascal100
#   watch-<server>             the same: each watch-<server> is a symlink to this script
#
# The port table below is the one place that records the ports. It must match the
# --mirror / SERIAL_MCP_MIRROR_TCP_PORT setting of each registration (claude mcp get <server>).
# Simulator mirrors (altairsim, pascal100) are watched with mirror-watch; serial-mcp mirrors
# with serial-watch. Each mirror accepts one watcher at a time.
#
# serial-shared is not here: its mirror is a read-write pty (SERIAL_MCP_MIRROR_TRANSPORT=pty)
# with no TCP port. Open the pty path that the server prints in a terminal program instead.
#
# serial-pascal (2425) was removed 2026-11-10 (user). Rule: `serial` holds ONE open port (its mirror is
# fixed on 2424); open every further port in serial-shared, whose pty mirror (/tmp/serial-mcp0, 1, ...)
# is read-write.
# Made 2026-11-10.

set -u

PORTS='
altairsim         2323  mirror-watch   altairsim machines
pascal100         2324  mirror-watch   pascal100 machines
serial            2424  serial-watch   real serial ports
'

name=$(basename "$0")
case $name in
    watch-*) server=${name#watch-} ;;
    *)       server=${1:-} ;;
esac

if [ -z "$server" ]; then
    echo "Usage: mcp-watch <server>   (or watch-<server>)"
    echo
    printf '%-18s %-5s %-13s %s\n' SERVER PORT TOOL "WHAT"
    echo "$PORTS" | awk 'NF { t=""; for (i=4; i<=NF; i++) t=t $i " "; printf "%-18s %-5s %-13s %s\n", $1, $2, $3, t }'
    exit 0
fi

line=$(echo "$PORTS" | awk -v s="$server" '$1 == s { print $2, $3; exit }')
if [ -z "$line" ]; then
    echo "mcp-watch: no mirror port recorded for \"$server\". Run mcp-watch with no argument for the list." >&2
    exit 1
fi
set -- $line
port=$1 tool=$2

# Find this script's own folder, through any watch-<server> symlink, for the fallback below.
self=$0
while [ -L "$self" ]; do
    link=$(readlink "$self")
    case $link in /*) self=$link ;; *) self=$(dirname "$self")/$link ;; esac
done
here=$(cd "$(dirname "$self")" && pwd)

case $tool in
    mirror-watch)
        # An installed mirror-watch first; else the mirror-watch.sh beside this script (the package's tools/).
        if command -v mirror-watch >/dev/null 2>&1; then exec mirror-watch localhost "$port"; fi
        if [ -x "$here/mirror-watch.sh" ]; then exec "$here/mirror-watch.sh" localhost "$port"; fi
        echo "mcp-watch: mirror-watch not found (not on PATH, and no mirror-watch.sh in $here)." >&2
        exit 1 ;;
    serial-watch)
        if command -v serial-watch >/dev/null 2>&1; then exec serial-watch localhost "$port"; fi
        echo "mcp-watch: serial-watch not found. It comes with serial-mcp-server (https://github.com/trgeuy/serial-mcp-server)." >&2
        exit 1 ;;
esac
