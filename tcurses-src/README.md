# tcurses

A small, modern, POSIX-flavored terminal UI library for TUS - the
same job as ncurses (initscr/box/addstr/refresh/getch, WINDOW
handles, color pairs, raw/cbreak/echo control via real termios), with
none of the terminfo database, no wide-character layer, and no
decades of legacy terminal quirks to abstract over, because it only
ever has to work against one terminal: TUS's own framebuffer console,
which already understands standard CUP/erase/SGR/alt-screen escapes.

The whole library is one header and one C file (~450 lines). It draws
into an in-memory virtual screen per `refresh()`/`wrefresh()` call and
diffs it against what was last actually painted, so redraws only ever
write the cells that changed - the same idea ncurses uses, just
without terminfo indirection or attribute-only optimizations that
don't matter on TUS's console.

See `tcurses.h` for the full API.
