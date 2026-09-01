# packages

This repository holds `.tpkg` packages for TUS, built with
[TPMT](https://github.com/qfdevel/TPMT) (TUS Package Making Tool) and
tested against the real `tpm` (TUS Package Manager) inside a booted
TUS. Nothing in here is speculative - every package under `tested/`
has actually been installed and run in TUS with `tpm install`, not
just built.

## Layout

- `tested/` - finished `.tpkg` files that have been built with TPMT,
  installed with `tpm` in a real booted TUS, and exercised (the
  program was actually run, not just unpacked). Each file is named
  after the package it contains, e.g. `tree.tpkg`.
- `HOWTOADDPACKAGE.md` - how to build and test a new package the same
  way, if you want to add one here.

## Currently packaged

- `tree.tpkg` - the `tree` directory-listing tool. It used to ship
  built into TUS by default; it was removed from TUS's own Makefile
  so it has to be installed like any other package now, and this is
  that package.

## Installing one of these in TUS

Copy the `.tpkg` file into TUS's rootfs (or onto a mounted `/mnt` on
a real disk install) and, from the TUS shell:

```
tus:/> tpm install /tmp/tree.tpkg
tus:/> tree /bin
```

`tpm list --installed` will then show it as installed.
