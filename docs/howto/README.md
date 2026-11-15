# How-to guides

This folder is for step-by-step guides that take a reader from the period disks to a working
system in pascal100. Each guide is written for a person alone at the simulator. You do not need
any real hardware.

## Build the Pascal-100 System

[`HOWTO-Build-the-System.md`](HOWTO-Build-the-System.md) takes the simulated Pascal-100 from the three disks that came with the boards to a full system, in
phases. Each phase ends with something that works:

| Phase | What you do | What you get |
|---|---|---|
| 1 | Install the simulator, set up the work folder and learn about the simulator | The `pascal100` command, a work folder, and CP/M on the screen |
| 2 | Get CP/M running and build a work disk | `CPMWORK.DSK` |
| 3 | Get single-sided, single-density (SSSD) Pascal running | `PASSD.DSK` |
| 4 | Get single-sided, double-density (SSDD) Pascal working | `PASDD.DSK` |
| Bonus 1 | Make a double-sided (DSDD) CP/M work disk | `CPMDS.DSK` |
| Bonus 2 | Make a DSDD Pascal work disk | `PASDS.DSK` |

The guide was tested on the simulator with pascal100 0.1.0, and the disks of phases 2 to 4 were
also tested on a real Pascal-100.

**The three starting disks.** The CompuPro CP/M disk is in this repository, at
[`disks/compupro-disk1/cpm22/CPM.DSK`](../../disks/compupro-disk1/cpm22/README.md). The two
Pascal-100 disks and the Pascal-100 manual are on the Internet Archive, at
<https://archive.org/details/cpm-files>; [`disks/README.md`](../../disks/README.md) gives the
file names and checksums. The guide calls the three disks `CPM.DSK`, `TOOLS.DSK` and
`PASCAL.DSK`, and expects them, with the machine file and the BIOS sources, in a `kit` folder.
The private bundle has that folder ready.

