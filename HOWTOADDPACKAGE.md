# How to add a package

This is the process used to build and test every package in
`tested/`. It assumes you already have TPMT built
(`/home/pi/projects/TPMT`, `make` from that directory produces
`build/tpmt`) and a working TUS checkout you can build and boot in
QEMU.

## 1. Get a static binary for the target program

TUS binaries are static x86_64 ELF, built against TUS's own musl
port. If the program already builds as part of TUS (like `tree` did
before it was pulled out), compile it standalone with the same flags
TUS's own Makefile uses for userspace tools:

```
clang -target x86_64-linux-gnu -m64 -ffreestanding -fno-stack-protector -fno-pic \
      -mno-red-zone -mgeneral-regs-only -O2 -Wno-shift-op-parentheses \
      -nostdinc -Imusl-out/usr/include -c userspace/<name>.c -o <name>.o

x86_64-linux-gnu-ld -m elf_x86_64 -static -e _start -Ttext 0x10000000 -o <name> \
    musl-out/usr/lib/crt1.o musl-out/usr/lib/crti.o <name>.o \
    -L musl-out/usr/lib -lc musl-out/usr/lib/crtn.o
```

Check the result with `file` - it should say a static, x86-64 ELF
executable.

## 2. Build the package with TPMT

```
tpmt init <name> <version>
```

This creates a `<name>/` directory with a `control` file and a
`data/` tree. Edit `control`:

```
Name: <name>
Version: <version>
Description: <one line>
```

Copy the binary into `data/` mirroring where it belongs on the target
filesystem (e.g. `data/bin/<name>`), and make sure it is executable
(`chmod 755`). Keep every path inside `data/` short - tpm's own tar
reader does not implement the ustar "prefix" field, so anything over
100 bytes will not read back correctly.

Then:

```
tpmt build <name>
tpmt inspect <name>_<version>.tpkg
```

`inspect` prints exactly what tpm will see - check it before testing.

## 3. Test it for real in TUS

Copy the `.tpkg` into TUS's `rootfs/tmp/`, rebuild TUS, and boot it in
QEMU. At the TUS shell:

```
tus:/> tpm install /tmp/<name>_<version>.tpkg
tus:/> <name>          # actually run it, don't just check the install log
tus:/> tpm list --installed
```

Confirm the install log looks right (select/unpack/install/setup) and
that the program actually runs and behaves correctly - not just that
`tpm` says it installed. Then remove the test copy from
`rootfs/tmp/` so TUS's rootfs stays clean.

## 4. Land it here

If you haven't done step 3 yet, or don't want to: rename the built
`.tpkg` to `<name>.tpkg` and put it under `untested/`. `tpm` will
still let people install it, but only after they confirm past its
"THIS PACKAGE HAS NOT BEEN TESTED" warning.

Once it has actually passed step 3 - installed and run for real with
`tpm` in a booted TUS - move (or copy) it into `tested/` instead, no
warning from there on.

Either way, rebuild that directory's index afterward so `tpm update`
picks up the change:

```
tpmt index tested
tpmt index untested
```

Commit the `.tpkg` together with the regenerated `Packages` file it
belongs under.
