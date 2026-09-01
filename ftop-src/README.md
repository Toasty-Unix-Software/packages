# ftop - Fast Top

A `top`-like live process viewer for TUS, built on
[tcurses](../tcurses-src). One source file, no dependencies beyond
tcurses (statically linked, so the installed binary needs nothing
else at runtime).

```
Fast Top - TUS                                                  q to quit
tasks: 5   uptime: 41s
mem: 39/475 MiB used

PID    PPID   PGID   UID    STATE   NAME
1      0      1      0      ready   tsh
2      1      1      0      ready   /bin/tussm
3      2      1      0      ready   errord
4      2      1      0      ready   bootd
8      1      1      0      run     /bin/ftop
```

## What it shows, and what it doesn't

TUS's scheduler keeps no per-task CPU-time accounting - there is
nothing in the kernel to read a `CPU%` column from, so `ftop` doesn't
show one. Making one up would just be a lie dressed as a stat. What it
does show is real:

- the live task table (PID/PPID/PGID/UID/state/name), read straight
  from the scheduler via `SYS_GETPROCS` - the same syscall `/bin/ps`
  uses
- system memory used/total, parsed from `/proc/meminfo`
- uptime, parsed from `/proc/uptime`

The screen redraws once a second. `q` or `Q` quits.

## Building

Same cross-compile recipe as every other TUS userspace tool (see
`HOWTOADDPACKAGE.md` at the repo root), plus `-I` to tcurses's
`include/` and linking `libtcurses.a`:

```
clang -target x86_64-linux-gnu -m64 -ffreestanding -fno-stack-protector -fno-pic \
      -mno-red-zone -mgeneral-regs-only -O2 -Wno-shift-op-parentheses \
      -nostdinc -Imusl-out/usr/include -I../tcurses-src/include \
      -c ftop.c -o ftop.o

x86_64-linux-gnu-ld -m elf_x86_64 -static -e _start -Ttext 0x10000000 -o ftop \
    musl-out/usr/lib/crt1.o musl-out/usr/lib/crti.o ftop.o ../tcurses-src/libtcurses.a \
    -L musl-out/usr/lib -lc musl-out/usr/lib/crtn.o
```

## Porting to another OS or project

See [PORTING.md](PORTING.md) - `ftop.c` has exactly two things in it
that are specific to TUS (the `SYS_GETPROCS` syscall wrapper and the
`/proc` file formats it parses); everything else is portable C using
tcurses's (or ncurses's) standard API.
