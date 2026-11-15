# What pascal100 is

`pascal100` simulates the **Digicomp Research Pascal-100** and the **IEEE 696 (S-100) boards**
around it. The Pascal-100 puts a Z80 and a Western Digital MicroEngine in one system. The Z80
boots the machine and runs CP/M; the MicroEngine runs UCSD Pascal p-code as its machine
language. A machine is a set of boards that plug into the bus. The program has built-in
machines, and you can describe your own machine in a machine file.

The machine runs the software of the period as it was sold: CompuPro CP/M 2.2 and UCSD
Pascal III.0 from 8″ floppy disks. The software is not patched, and it cannot tell that the
machine is simulated.

> **pascal100 comes from `altairsim`**, a simulator of the MITS Altair 8800 by Patrick
> Linstruth. This manual was adapted for pascal100, but the separate Monitor and Debugger
> guides still have altairsim examples, and some text can still be out of date. When a manual
> and the program do not agree, `HELP` and `SHOW BOARDS` at the monitor are correct.

## What you can do with it

- **Run period software.** Boot CompuPro CP/M, or boot UCSD Pascal on the MicroEngine from a
  disk that you supply.
- **Write software for the machine, and find its bugs.** You can stop the machine at an
  instruction or a bus cycle that you choose. You can see which board answered and which board
  did not. When you give the machine the same input again, you get the same result.
- **Test a driver before you have the hardware.** Write a driver for a board that the program
  has, and run it against that board.
- **Connect the machine to your computer.** A serial board can use your terminal, a terminal
  window, a TCP socket or a real serial port. A line-printer board can print on the printer on
  your desk.
- **Let an AI assistant use the machine.** An assistant can boot the machine, type at it and
  debug a program through the MCP server.

## What is in the program

- **The Pascal-100 processor board,** with two live processors: the Z80 and the MicroEngine.
  One of them has the bus at a time, as on the real board.
- **Other processors:** a Z80 board and an 8085 board. Each processor gives the correct flags,
  including its undocumented behavior.
- **Period boards:** CompuPro Disk 1, Disk 1A, Interfacer 1 and System Support 1; other disk
  controllers, serial boards, memory boards, a clock and a line-printer controller. The boards
  chapter describes each one, and `SHOW BOARDS` at the monitor lists them.
- **The monitor.** The `pascal100>` prompt controls the machine while it is stopped. From the
  monitor, you can set breakpoints, step one instruction at a time, disassemble, change memory,
  and trace the bus. *The Monitor* and *The Debugger* are two documents that ship beside this
  manual, and they describe these commands.
- **File transfer** between your computer and CP/M. A board that the period did not have moves
  the files, and the guest can use only the one folder that you choose. At the `A>` prompt, you
  type `HDIR`, `R` and `W`. These are ordinary CP/M programs, like `PIP` or `STAT`.

## What it does not do

- **You can save the state of the machine, but you cannot replay a session.** `SNAPSHOT` writes
  the whole state of the machine to a file, and `RESTORE` reads it back. You cannot record a
  session and step backward through it.
- **Not every S-100 board is here.** The boards chapter lists the boards that are.
- **The timing is correct for the software, but it is not a circuit simulation.** Each
  instruction takes the correct number of T-states. The program does not model propagation
  delay, analog behavior or the rotation of a disk.
- **One Pascal disk comes with the program**, in `examples/pascal`. It is not covered by the
  MIT license of pascal100.

## Where to start

There is no installer, and there is nothing to set up. Unzip the package and run the program.

- To boot CP/M now, go to the *Quick start*. It takes one command.
- To learn what came in the package, read the next chapter.
- If the program does not start, read *Running it*.
