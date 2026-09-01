# Porting ftop to another OS or project

`ftop.c` is deliberately small (~175 lines) and almost all of it is
plain POSIX C against the tcurses/ncurses API, which both implement
the same calls (`initscr`, `cbreak`, `noecho`, `curs_set`, `werase`,
`attron`/`attroff`, `mvprintw`, `wmove`, `whline`, `refresh`, `getch`,
`endwin`). The only TUS-specific pieces are isolated in two small
functions plus one struct definition, all near the top of the file.
Replace those and the rest carries over unchanged.

## 1. The curses library

`#include <tcurses.h>` is TUS's own lightweight curses (see
[../tcurses-src](../tcurses-src)). On a system that already has real
ncurses, this is a one-line swap:

```c
#include <curses.h>   /* or <ncurses.h> */
```

link with `-lncurses` instead of `libtcurses.a`, and everything else
in `ftop.c` compiles as-is - `ftop` only uses API that both
implement. If your curses (real or otherwise) doesn't have
`curs_set()`, wrap it in `#ifdef`.

## 2. Getting the process list

TUS has no `/proc/<pid>/` directory tree and no Linux-compatible
process-listing syscall, so `ftop` calls a TUS-only syscall,
`SYS_GETPROCS`, through a raw `int $0x80`:

```c
#define SYS_GETPROCS 83

struct tus_procinfo {
    uint32_t pid, ppid, pgid, uid, state;
    char name[32];
};

static long tus_syscall2(long n, long a1, long a2) { /* raw int $0x80 */ }

long n = tus_syscall2(SYS_GETPROCS, (long)procs, sizeof(procs));
```

On a system with a real `/proc` (Linux, most other UNIX-likes),
replace this whole block with a directory walk instead:

```c
DIR *d = opendir("/proc");
struct dirent *e;
while ((e = readdir(d))) {
    if (!isdigit((unsigned char)e->d_name[0])) continue;
    /* pid = atoi(e->d_name); read /proc/<pid>/stat or /status for
     * ppid/pgid/uid/state/name, fill in a struct with the same
     * fields ftop's draw() already expects */
}
```

Keep the field names the same (`pid`, `ppid`, `pgid`, `uid`, `state`,
`name`) and `draw()` in `ftop.c` needs no changes at all - it only
touches `procs[i].<field>`, never anything syscall-shaped. On BSD,
`sysctl(KERN_PROC, ...)` or `kvm_getprocs()` fills the same struct;
on macOS, `sysctl(KERN_PROC_ALL, ...)`.

`state_name()` maps TUS's four-state scheduler (ready/running/zombie/
stopped) to short strings for the STATE column - adjust the mapping
to whatever your OS's process-state enum actually has (Linux, for
example, has running/sleeping/disk-wait/zombie/stopped/traced).

## 3. Memory and uptime

`read_meminfo()` and `read_uptime_secs()` parse TUS's
`/proc/meminfo` and `/proc/uptime`, which happen to already look
almost exactly like Linux's real files of the same name (`MemTotal:
NNNN kB`, and a plain "seconds seconds" line) - so on Linux itself,
**these two functions need no changes**. On a system with a
differently-shaped `/proc`, or none at all, replace them:

- Linux: no change needed (or use `sysinfo(2)` instead if you'd
  rather not parse text at all - `struct sysinfo` gives you
  `totalram`/`freeram` and `uptime` directly).
- BSD/macOS: `sysctlbyname("hw.physmem", ...)` /
  `sysctlbyname("vm.stats.vm.v_free_count", ...)` for memory,
  `sysctlbyname("kern.boottime", ...)` compared against
  `gettimeofday()` for uptime.
- Anything else with no memory-stats API worth reading: keep
  `read_meminfo()` returning failure (`have_mem = 0`) - `draw()`
  already handles that by printing "mem: unknown" instead of
  crashing or guessing.

## 4. What never needs to change

`by_pid()` (the sort comparator), the entire `draw()` layout and
formatting, and `main()`'s `poll()`-based redraw loop are all plain
POSIX/curses code with nothing TUS-specific in them. `poll()` on
fd 0 with a 1000 ms timeout is what makes the redraw interval
interruptible by a keypress instead of blocking on `getch()` for a
full second; any POSIX system has `poll()`.
