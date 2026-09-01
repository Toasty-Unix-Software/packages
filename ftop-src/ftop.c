/*
 * ftop.c - Fast Top: a live process viewer for TUS, built on tcurses.
 *
 * TUS's scheduler keeps no per-task CPU-time accounting (see
 * kernel/vfs/procfs.c's own loadavg/stat comments), so this does not
 * invent a CPU% column - it shows what the kernel actually tracks:
 * the live task table (SYS_GETPROCS, the same syscall /bin/ps uses)
 * plus system-wide memory and uptime from /proc, refreshed once a
 * second until 'q'.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <tcurses.h>

#define SYS_GETPROCS 83
#define TASK_NAME_MAX 32
#define PROCS_MAX 128

struct tus_procinfo {
    uint32_t pid;
    uint32_t ppid;
    uint32_t pgid;
    uint32_t uid;
    uint32_t state;
    char name[TASK_NAME_MAX];
};

static long tus_syscall2(long n, long a1, long a2) {
    long ret;
    register long r10 __asm__("r10") = 0;
    register long r8 __asm__("r8") = 0;
    register long r9 __asm__("r9") = 0;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret), "+r"(rdi), "+r"(rsi), "+r"(rdx),
                       "+r"(r10), "+r"(r8), "+r"(r9)
                     : "a"(n)
                     : "memory", "cc");
    return ret;
}

static const char *state_name(uint32_t state) {
    switch (state) {
    case 1: return "ready";
    case 2: return "run";
    case 3: return "zombie";
    case 4: return "stop";
    default: return "?";
    }
}

static long read_uptime_secs(void) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) {
        return -1;
    }
    long secs = -1;
    if (fscanf(f, "%ld", &secs) != 1) {
        secs = -1;
    }
    fclose(f);
    return secs;
}

/* /proc/meminfo lines look like "MemTotal: 12345 kB" - each one
 * starts with its label, so a plain numeric fscanf() never matches
 * the first token and reads nothing at all. */
static long parse_kb_line(const char *line, const char *key) {
    size_t klen = strlen(key);
    if (strncmp(line, key, klen) != 0 || line[klen] != ':') {
        return -1;
    }
    return atol(line + klen + 1);
}

static int read_meminfo(long *total_kb, long *free_kb) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        return -1;
    }
    char line[128];
    *total_kb = -1;
    *free_kb = -1;
    while (fgets(line, sizeof(line), f)) {
        long v = parse_kb_line(line, "MemTotal");
        if (v >= 0) {
            *total_kb = v;
            continue;
        }
        v = parse_kb_line(line, "MemFree");
        if (v >= 0) {
            *free_kb = v;
        }
    }
    fclose(f);
    return (*total_kb >= 0 && *free_kb >= 0) ? 0 : -1;
}

static int by_pid(const void *a, const void *b) {
    const struct tus_procinfo *pa = a, *pb = b;
    return (int)pa->pid - (int)pb->pid;
}

static void draw(void) {
    static struct tus_procinfo procs[PROCS_MAX];
    long n = tus_syscall2(SYS_GETPROCS, (long)procs, sizeof(procs));
    if (n < 0) {
        n = 0;
    }
    qsort(procs, (size_t)n, sizeof(procs[0]), by_pid);

    long mem_total, mem_free;
    int have_mem = read_meminfo(&mem_total, &mem_free) == 0;
    long uptime_s = read_uptime_secs();

    werase(stdscr);

    attron(A_BOLD);
    mvprintw(0, 0, "Fast Top - TUS");
    attroff(A_BOLD);
    mvprintw(0, COLS - 20, "q to quit");

    mvprintw(1, 0, "tasks: %ld   uptime: %lds", n, uptime_s < 0 ? 0 : uptime_s);
    if (have_mem) {
        long used = mem_total - mem_free;
        mvprintw(2, 0, "mem: %ld/%ld MiB used", used / 1024, mem_total / 1024);
    } else {
        mvprintw(2, 0, "mem: unknown");
    }

    attron(A_REVERSE);
    wmove(stdscr, 4, 0);
    whline(stdscr, ' ', COLS);
    mvprintw(4, 0, "%-6s %-6s %-6s %-6s %-7s %s", "PID", "PPID", "PGID",
             "UID", "STATE", "NAME");
    attroff(A_REVERSE);

    int row = 5;
    for (long i = 0; i < n && row < LINES; i++, row++) {
        mvprintw(row, 0, "%-6u %-6u %-6u %-6u %-7s %s", procs[i].pid,
                 procs[i].ppid, procs[i].pgid, procs[i].uid,
                 state_name(procs[i].state), procs[i].name);
    }

    refresh();
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    draw();
    for (;;) {
        struct pollfd pfd = { .fd = 0, .events = POLLIN, .revents = 0 };
        int r = poll(&pfd, 1, 1000);
        if (r > 0 && (pfd.revents & POLLIN)) {
            int ch = getch();
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }
        draw();
    }

    endwin();
    return 0;
}
