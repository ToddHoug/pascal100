# pascal100 — User Manual

These files are the source of the *pascal100 User Manual*. The release package has the manual as
one document, `pascal100-manual.pdf`. Read that document if you have the package. This page
lists the chapters in the order of the manual, so that you can read them here.

**Start with the [Quick start](quick-start.md)**, which boots CP/M with one command. To learn
what the program is first, read [What pascal100 is](introduction.md).

## The chapters

1. [What pascal100 is](introduction.md): what the program does, and what it does not do.
2. [What is in the package](package.md): the program, the documents, the built-in machines, the
   examples, and what is not in the package.
3. [Running it](running.md): unzip the package and run the program.
4. [Quick start](quick-start.md): CP/M with one command. Stop with `Ctrl-E`, continue with
   `RUN`, leave with `QUIT`.
5. [Quick reference](ref/cheatsheet.md): the command line, every command, the outline of a
   machine file, and the boards, on one page.
6. [Machines](machines.md): the command line, the built-in machines, and where a relative path
   starts.
7. [Configuring a machine](configuring.md): the TOML format, in full.
8. [Boards](boards.md): what each board is, and what it is for.
9. [Disks](disks.md): `MOUNT`, disk formats, and the track buffer.
10. [Serial ports, sockets and telnet](serial.md): how to connect a board to your terminal, a
    TCP port or a real serial port.
11. [Moving files in and out](file-transfer.md): `HDIR`, `R` and `W` at the CP/M prompt.
12. [Worked examples](examples.md): complete sessions, from start to end.
13. [The MCP server](mcp.md): how an AI assistant controls the machine.
14. [Troubleshooting](troubleshooting.md): the most common problems, and what to do.
15. [Glossary](glossary.md): S-100, PHANTOM\*, hard-sector, BDOS and other terms.
16. [Boards and their properties](ref/boards.md): every board, every key and every default.
17. [The built-in machines](ref/machines.md): every built-in machine, and what is in it.

## Other documents

Two more documents ship beside the manual. They describe the `pascal100>` prompt and its
debugger, which control the program, not the simulated hardware.

- [*The Monitor*](../monitor/monitor.md) (`pascal100-monitor.pdf`): the `pascal100>` prompt,
  short forms of commands, the number rule, board names and STOP. It also has the command
  reference, with every `pascal100>` command and its usage and examples.
- [*The Debugger*](../debugger/debugging.md) (`pascal100-debugger.pdf`): breakpoints, stepping,
  disassembly, and how to look at the bus.

To build a board of your own, you need the source and a different document, the **Developer
Guide**. It is not in the package. It is with the source.
