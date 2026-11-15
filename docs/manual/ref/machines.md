<!-- GENERATED FROM THE PROGRAM ITSELF. Do not edit by hand.
     Every default, range and description below is printed from the same tables the
     monitor resolves against, so it cannot disagree with the program you are running. -->

# The built-in machines

A built-in is a machine file that lives **inside the binary** — the same format you
would write yourself, with nothing special about it. Name one and you get it in any
directory on earth:

```
$ pascal100 basic4k
```

`pascal100 --list` prints this table, and `pascal100 -x 'SHOW MACHINE' <name>` shows
what is actually in one.

| Machine | What it is |
|---|---|
| `default` | The machine you get when you name none: a CompuPro-style IEEE 696 workbench. |

