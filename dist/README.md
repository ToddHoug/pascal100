# `dist/` — the release staging and collection point

> **Not adapted yet (2026-11-09).** This is altairsim's release process, kept for the pascal100
> release step. The program, package and signing names say pascal100, but the rest is
> altairsim's: release tags are still `vX.Y.Z` (pascal100 uses `pascal100-vX.Y.Z`), the PDF
> names, the four machines, Apple signing and the download site. Do not run a release from it
> until the release step has reviewed it.

This directory is where a release is assembled. Two things land here, and **neither is
tracked** (`.gitignore` ignores everything under `dist/` except this README):

1. **What this machine builds.** `tools/build-package.sh` stages
   `pascal100-<ver>-<target>/` here and writes the archive beside it
   (`pascal100-<ver>-<target>.tar.gz`, or `.zip` for Windows).

2. **What the other machines deliver.** In `DISTRIBUTION.md`'s terms this is the
   **coordinator**, and `dist/` is where the three workers (macOS Intel, Windows, Linux)
   `scp` their finished archives — each builds its own platform natively, over an
   anonymous `https://` clone, and hands the result here. The coordinator then uploads all
   four to the GitHub release. See `DISTRIBUTION.md` §4 and §6.

The directory is committed empty (this README is the only tracked file) because a worker
may deliver before the coordinator has built anything, so `dist/` has to exist on a fresh
checkout.
