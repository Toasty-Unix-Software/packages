# packages

This is TUS's official package repository, served over GitHub Pages
at `https://toasty-unix-software.github.io/packages/`. It holds
`.tpkg` packages built with
[TPMT](https://github.com/qfdevel/TPMT) (TUS Package Making Tool),
and this is what `tpm` (TUS Package Manager) points at by default -
see `rootfs/etc/tpm/source.list` in the TUS tree itself.

## Layout

- `tested/` - packages that have actually been installed and run with
  a real `tpm` inside a booted TUS before being added here. `tpm
  install <name>` from this directory just works, no warning.
- `untested/` - packages that have only been built with TPMT and
  never installed for real. `tpm` still allows installing these, but
  first prints:

  ```
  THIS PACKAGE HAS NOT BEEN TESTED. DO YOU STILL WANT TO INSTALL IT? (y/N)
  ```

  and refuses unless the user types `y`.
- `Packages` inside each of those two directories is the index
  `tpm update` fetches - one `name version filename` line per
  package, rebuilt with `tpmt index <dir>` whenever a package is
  added or changed.
- `HOWTOADDPACKAGE.md` - how to build and test a new package the same
  way, if you want to add one here.

## Currently packaged

- `tree.tpkg` (in `tested/`) - the `tree` directory-listing tool. It
  used to ship built into TUS by default; it was removed from TUS's
  own Makefile so it has to be installed like any other package now,
  and this is that package. Verified installed and run with a real
  `tpm` in a booted TUS.

## Installing one of these in TUS

With the repo configured in `/etc/tpm/source.list` (it is, by
default):

```
tus:/> tpm update
tus:/> tpm install tree
```

Or without going through the network at all - copy the `.tpkg` file
into TUS's rootfs (or onto a mounted `/mnt` on a real disk install)
and:

```
tus:/> tpm install /tmp/tree.tpkg
tus:/> tree /bin
```

`tpm list --installed` will then show it as installed.
