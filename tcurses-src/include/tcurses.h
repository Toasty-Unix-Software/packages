#ifndef TCURSES_H
#define TCURSES_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int chtype;
typedef int bool_t;

#define OK   0
#define ERR  (-1)

#define A_NORMAL    0x00000000u
#define A_BOLD      0x00010000u
#define A_REVERSE   0x00020000u
#define A_UNDERLINE 0x00040000u
#define A_ATTR_MASK 0xFFFF0000u
#define A_CHARTEXT  0x0000FFFFu

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define COLOR_PAIR(n) (((chtype)(n) & 0xFF) << 24)
#define PAIR_NUMBER(a) (((a) >> 24) & 0xFF)

typedef struct _tcwin WINDOW;

extern WINDOW *stdscr;
extern int LINES;
extern int COLS;

#define getmaxyx(w, y, x) ((y) = tc_win_height(w), (x) = tc_win_width(w))
#define getbegyx(w, y, x) ((y) = tc_win_begy(w), (x) = tc_win_begx(w))
#define getyx(w, y, x)    ((y) = tc_win_cury(w), (x) = tc_win_curx(w))

WINDOW *initscr(void);
int endwin(void);
int isendwin(void);

WINDOW *newwin(int nlines, int ncols, int begy, int begx);
int delwin(WINDOW *win);
int mvwin(WINDOW *win, int begy, int begx);

int cbreak(void);
int nocbreak(void);
int raw(void);
int noraw(void);
int echo(void);
int noecho(void);
int keypad(WINDOW *win, int enable);
int nodelay(WINDOW *win, int enable);
int curs_set(int visibility);
int nl(void);
int nonl(void);

int start_color(void);
int has_colors(void);
int init_pair(short pair, short fg, short bg);

int wmove(WINDOW *win, int y, int x);
#define move(y, x) wmove(stdscr, y, x)

int waddch(WINDOW *win, chtype ch);
int mvwaddch(WINDOW *win, int y, int x, chtype ch);
#define addch(ch) waddch(stdscr, ch)
#define mvaddch(y, x, ch) mvwaddch(stdscr, y, x, ch)

int waddstr(WINDOW *win, const char *s);
int mvwaddstr(WINDOW *win, int y, int x, const char *s);
#define addstr(s) waddstr(stdscr, s)
#define mvaddstr(y, x, s) mvwaddstr(stdscr, y, x, s)

int wprintw(WINDOW *win, const char *fmt, ...);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int vwprintw(WINDOW *win, const char *fmt, va_list ap);
#define printw(...) wprintw(stdscr, __VA_ARGS__)
#define mvprintw(y, x, ...) mvwprintw(stdscr, y, x, __VA_ARGS__)

int wclear(WINDOW *win);
int werase(WINDOW *win);
int wclrtoeol(WINDOW *win);
int wclrtobot(WINDOW *win);
#define clear() wclear(stdscr)
#define erase() werase(stdscr)
#define clrtoeol() wclrtoeol(stdscr)
#define clrtobot() wclrtobot(stdscr)

int wattron(WINDOW *win, chtype attr);
int wattroff(WINDOW *win, chtype attr);
int wattrset(WINDOW *win, chtype attr);
#define attron(a) wattron(stdscr, a)
#define attroff(a) wattroff(stdscr, a)
#define attrset(a) wattrset(stdscr, a)

int box(WINDOW *win, chtype vert, chtype horiz);
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br);
int whline(WINDOW *win, chtype ch, int n);
int wvline(WINDOW *win, chtype ch, int n);

int wrefresh(WINDOW *win);
int refresh(void);
int wnoutrefresh(WINDOW *win);
int doupdate(void);
int touchwin(WINDOW *win);

int wgetch(WINDOW *win);
#define getch() wgetch(stdscr)

int tc_win_height(WINDOW *win);
int tc_win_width(WINDOW *win);
int tc_win_begy(WINDOW *win);
int tc_win_begx(WINDOW *win);
int tc_win_cury(WINDOW *win);
int tc_win_curx(WINDOW *win);

#define KEY_UP     0x101
#define KEY_DOWN   0x102
#define KEY_RIGHT  0x103
#define KEY_LEFT   0x104
#define KEY_HOME   0x105
#define KEY_END    0x106
#define KEY_BACKSPACE 0x107
#define KEY_DC     0x108
#define KEY_RESIZE 0x109

#ifdef __cplusplus
}
#endif

#endif
