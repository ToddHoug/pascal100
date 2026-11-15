# machines/

Each `.toml` file in this directory is a built-in machine. A machine file describes a whole
simulated machine: the processor board, the memory, the other boards, and the commands that
run at startup.

**There is one built-in machine: `default.toml`.** It is what a user gets when they name no
machine. Every other machine is a complete file in [`examples/`](../examples/), with every board
written out (decision 2026-11-14): a user reads one file and sees the whole machine. Until
2026-11-14 `disk1`, `disk1a`, `pascal100` and `tarbell` were built in too. They are now test
fixtures (`tests/media/compupro/machine-disk1*.toml`, `tests/acceptance/machine-tarbell.toml`,
`tests/media/pascal100/machine-pascal100.toml`) and the examples.

The build compiles these files into the program. A user starts one by name and needs no file
on disk:

```sh
pascal100 --list        # show the built-in machine
pascal100 default       # start the machine from default.toml
```

## How a file gets into the program

1. [`cmake/embed_machines.cmake`](../cmake/embed_machines.cmake) finds every `*.toml` here.
2. The script copies each file, byte for byte, into `machines_generated.cpp`.
3. The program parses that text with the same loader that reads a machine file from disk.

The name of the machine is the file name without `.toml`. The first comment line of the file
is the one-line description that `pascal100 --list` prints.

## Add a machine

Add a new machine to [`examples/`](../examples/) as a complete file, not here. If a built-in is
really needed:

1. Add `<name>.toml` to this directory.
2. Write the description on the first line, as a comment.
3. Build. The new machine is in `pascal100 --list`.
4. Run `./build/altair_tests machines`. That suite loads every built-in machine, and it checks
   the number of built-ins, so change that check too.

A built-in machine has no folder of its own, so it mounts only built-in media, such as
`builtin:disk1` from [`roms/`](../roms/) (`SHOW ROMS` lists them). A machine that needs a disk beside it
belongs in [`examples/`](../examples/).

## Read more

- [Machines](../docs/manual/machines.md) and [Configuring a machine](../docs/manual/configuring.md)
  in the *User Manual*, for the format.
- [`docs/config.md`](../docs/config.md) for why the format has this shape.
