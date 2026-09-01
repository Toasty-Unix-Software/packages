#include "tcurses.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

struct _tcwin {
    int begy, begx;
    int maxy, maxx;
    int cury, curx;
    chtype attr;
    int keypad_on;
    int nodelay_on;
    chtype *cells;
};

WINDOW *stdscr;
int LINES;
int COLS;

static chtype *g_virt;
static chtype *g_phys;
static int g_rows, g_cols;

static struct termios g_orig_tio;
static int g_have_orig_tio;
static int g_raw_mode;
static int g_echo_mode = 1;
static int g_ended = 1;

static short g_pair_fg[64];
static short g_pair_bg[64];

static void apply_tio(void) {
    struct termios tio;
    if (tcgetattr(0, &tio) != 0) {
        return;
    }
    if (g_raw_mode) {
        tio.c_lflag &= ~(tcflag_t)(ICANON | ISIG);
    } else {
        tio.c_lflag &= ~(tcflag_t)ICANON;
        tio.c_lflag |= ISIG;
    }
    if (g_echo_mode) {
        tio.c_lflag |= ECHO;
    } else {
        tio.c_lflag &= ~(tcflag_t)ECHO;
    }
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &tio);
}

static int query_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
    *rows = 24;
    *cols = 80;
    return -1;
}

WINDOW *initscr(void) {
    if (!g_ended) {
        return stdscr;
    }
    query_size(&g_rows, &g_cols);
    LINES = g_rows;
    COLS = g_cols;

    g_have_orig_tio = (tcgetattr(0, &g_orig_tio) == 0);
    g_raw_mode = 0;
    g_echo_mode = 0;
    apply_tio();

    g_virt = calloc((size_t)g_rows * (size_t)g_cols, sizeof(chtype));
    g_phys = malloc((size_t)g_rows * (size_t)g_cols * sizeof(chtype));
    for (size_t i = 0; i < (size_t)g_rows * (size_t)g_cols; i++) {
        g_virt[i] = ' ';
        g_phys[i] = (chtype)~0u; /* force full paint on first doupdate() */
    }

    stdscr = newwin(g_rows, g_cols, 0, 0);

    printf("\033[?1049h\033[?25h\033[2J\033[H");
    fflush(stdout);

    g_ended = 0;
    return stdscr;
}

int endwin(void) {
    if (g_ended) {
        return OK;
    }
    printf("\033[0m\033[?1049l");
    fflush(stdout);
    if (g_have_orig_tio) {
        tcsetattr(0, TCSAFLUSH, &g_orig_tio);
    }
    g_ended = 1;
    return OK;
}

int isendwin(void) {
    return g_ended;
}

WINDOW *newwin(int nlines, int ncols, int begy, int begx) {
    if (nlines <= 0) {
        nlines = g_rows - begy;
    }
    if (ncols <= 0) {
        ncols = g_cols - begx;
    }
    WINDOW *w = calloc(1, sizeof(WINDOW));
    if (!w) {
        return NULL;
    }
    w->begy = begy;
    w->begx = begx;
    w->maxy = nlines;
    w->maxx = ncols;
    w->cury = 0;
    w->curx = 0;
    w->attr = A_NORMAL;
    w->keypad_on = 0;
    w->nodelay_on = 0;
    w->cells = malloc((size_t)nlines * (size_t)ncols * sizeof(chtype));
    for (size_t i = 0; i < (size_t)nlines * (size_t)ncols; i++) {
        w->cells[i] = ' ';
    }
    return w;
}

int delwin(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    free(win->cells);
    free(win);
    return OK;
}

int mvwin(WINDOW *win, int begy, int begx) {
    if (!win) {
        return ERR;
    }
    win->begy = begy;
    win->begx = begx;
    return OK;
}

int cbreak(void) { g_raw_mode = 0; apply_tio(); return OK; }
int nocbreak(void) { g_raw_mode = 0; apply_tio(); return OK; }
int raw(void) { g_raw_mode = 1; apply_tio(); return OK; }
int noraw(void) { g_raw_mode = 0; apply_tio(); return OK; }
int echo(void) { g_echo_mode = 1; apply_tio(); return OK; }
int noecho(void) { g_echo_mode = 0; apply_tio(); return OK; }

int keypad(WINDOW *win, int enable) {
    if (!win) {
        return ERR;
    }
    win->keypad_on = enable;
    return OK;
}

int nodelay(WINDOW *win, int enable) {
    if (!win) {
        return ERR;
    }
    win->nodelay_on = enable;
    return OK;
}

int curs_set(int visibility) {
    if (visibility == 0) {
        printf("\033[?25l");
    } else {
        printf("\033[?25h");
    }
    fflush(stdout);
    return OK;
}

int nl(void) { return OK; }
int nonl(void) { return OK; }

int start_color(void) {
    for (int i = 0; i < 64; i++) {
        g_pair_fg[i] = COLOR_WHITE;
        g_pair_bg[i] = COLOR_BLACK;
    }
    return OK;
}

int has_colors(void) {
    return 1;
}

int init_pair(short pair, short fg, short bg) {
    if (pair < 0 || pair >= 64) {
        return ERR;
    }
    g_pair_fg[pair] = fg;
    g_pair_bg[pair] = bg;
    return OK;
}

int wmove(WINDOW *win, int y, int x) {
    if (!win) {
        return ERR;
    }
    if (y < 0 || y >= win->maxy || x < 0 || x >= win->maxx) {
        return ERR;
    }
    win->cury = y;
    win->curx = x;
    return OK;
}

static void put_cell(WINDOW *win, int y, int x, chtype ch) {
    if (y < 0 || y >= win->maxy || x < 0 || x >= win->maxx) {
        return;
    }
    win->cells[y * win->maxx + x] = (ch & (A_CHARTEXT | A_ATTR_MASK)) | (win->attr & ~A_CHARTEXT);
}

int waddch(WINDOW *win, chtype ch) {
    if (!win) {
        return ERR;
    }
    unsigned char c = (unsigned char)(ch & A_CHARTEXT);
    chtype attrs = (ch & A_ATTR_MASK) | (win->attr & A_ATTR_MASK);
    if (c == '\n') {
        win->curx = 0;
        win->cury++;
    } else {
        put_cell(win, win->cury, win->curx, (chtype)c | attrs);
        win->curx++;
        if (win->curx >= win->maxx) {
            win->curx = 0;
            win->cury++;
        }
    }
    if (win->cury >= win->maxy) {
        win->cury = win->maxy - 1;
    }
    return OK;
}

int mvwaddch(WINDOW *win, int y, int x, chtype ch) {
    if (wmove(win, y, x) != OK) {
        return ERR;
    }
    return waddch(win, ch);
}

int waddstr(WINDOW *win, const char *s) {
    if (!win || !s) {
        return ERR;
    }
    while (*s) {
        waddch(win, (unsigned char)*s);
        s++;
    }
    return OK;
}

int mvwaddstr(WINDOW *win, int y, int x, const char *s) {
    if (wmove(win, y, x) != OK) {
        return ERR;
    }
    return waddstr(win, s);
}

int vwprintw(WINDOW *win, const char *fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    return waddstr(win, buf);
}

int wprintw(WINDOW *win, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vwprintw(win, fmt, ap);
    va_end(ap);
    return r;
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...) {
    if (wmove(win, y, x) != OK) {
        return ERR;
    }
    va_list ap;
    va_start(ap, fmt);
    int r = vwprintw(win, fmt, ap);
    va_end(ap);
    return r;
}

int wclear(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    for (int i = 0; i < win->maxy * win->maxx; i++) {
        win->cells[i] = ' ';
    }
    win->cury = 0;
    win->curx = 0;
    return OK;
}

int werase(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    for (int i = 0; i < win->maxy * win->maxx; i++) {
        win->cells[i] = ' ';
    }
    return OK;
}

int wclrtoeol(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    for (int x = win->curx; x < win->maxx; x++) {
        win->cells[win->cury * win->maxx + x] = ' ';
    }
    return OK;
}

int wclrtobot(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    wclrtoeol(win);
    for (int y = win->cury + 1; y < win->maxy; y++) {
        for (int x = 0; x < win->maxx; x++) {
            win->cells[y * win->maxx + x] = ' ';
        }
    }
    return OK;
}

int wattron(WINDOW *win, chtype attr) {
    if (!win) {
        return ERR;
    }
    win->attr |= attr;
    return OK;
}

int wattroff(WINDOW *win, chtype attr) {
    if (!win) {
        return ERR;
    }
    win->attr &= ~attr;
    return OK;
}

int wattrset(WINDOW *win, chtype attr) {
    if (!win) {
        return ERR;
    }
    win->attr = attr;
    return OK;
}

int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br) {
    if (!win) {
        return ERR;
    }
    if (!ls) ls = '|';
    if (!rs) rs = '|';
    if (!ts) ts = '-';
    if (!bs) bs = '-';
    if (!tl) tl = '+';
    if (!tr) tr = '+';
    if (!bl) bl = '+';
    if (!br) br = '+';

    int h = win->maxy, w = win->maxx;
    for (int x = 1; x < w - 1; x++) {
        put_cell(win, 0, x, ts | win->attr);
        put_cell(win, h - 1, x, bs | win->attr);
    }
    for (int y = 1; y < h - 1; y++) {
        put_cell(win, y, 0, ls | win->attr);
        put_cell(win, y, w - 1, rs | win->attr);
    }
    put_cell(win, 0, 0, tl | win->attr);
    put_cell(win, 0, w - 1, tr | win->attr);
    put_cell(win, h - 1, 0, bl | win->attr);
    put_cell(win, h - 1, w - 1, br | win->attr);
    return OK;
}

int box(WINDOW *win, chtype vert, chtype horiz) {
    return wborder(win, vert, vert, horiz, horiz, 0, 0, 0, 0);
}

int whline(WINDOW *win, chtype ch, int n) {
    if (!win) {
        return ERR;
    }
    if (!ch) ch = '-';
    for (int i = 0; i < n && win->curx + i < win->maxx; i++) {
        put_cell(win, win->cury, win->curx + i, ch | win->attr);
    }
    return OK;
}

int wvline(WINDOW *win, chtype ch, int n) {
    if (!win) {
        return ERR;
    }
    if (!ch) ch = '|';
    for (int i = 0; i < n && win->cury + i < win->maxy; i++) {
        put_cell(win, win->cury + i, win->curx, ch | win->attr);
    }
    return OK;
}

int wnoutrefresh(WINDOW *win) {
    if (!win) {
        return ERR;
    }
    for (int y = 0; y < win->maxy; y++) {
        int ty = win->begy + y;
        if (ty < 0 || ty >= g_rows) {
            continue;
        }
        for (int x = 0; x < win->maxx; x++) {
            int tx = win->begx + x;
            if (tx < 0 || tx >= g_cols) {
                continue;
            }
            g_virt[ty * g_cols + tx] = win->cells[y * win->maxx + x];
        }
    }
    return OK;
}

static void emit_sgr(chtype attr) {
    static chtype last_attr = (chtype)~0u;
    if (attr == last_attr) {
        return;
    }
    last_attr = attr;
    printf("\033[0m");
    if (attr & A_BOLD) {
        printf("\033[1m");
    }
    if (attr & A_UNDERLINE) {
        printf("\033[4m");
    }
    if (attr & A_REVERSE) {
        printf("\033[7m");
    }
    int pair = (int)PAIR_NUMBER(attr);
    if (pair > 0 && pair < 64) {
        printf("\033[%dm\033[%dm", 30 + g_pair_fg[pair], 40 + g_pair_bg[pair]);
    }
}

int doupdate(void) {
    int last_y = -1, last_x = -1;
    for (int y = 0; y < g_rows; y++) {
        for (int x = 0; x < g_cols; x++) {
            size_t idx = (size_t)y * g_cols + x;
            if (g_virt[idx] == g_phys[idx]) {
                continue;
            }
            if (y != last_y || x != last_x) {
                printf("\033[%d;%dH", y + 1, x + 1);
            }
            emit_sgr(g_virt[idx] & A_ATTR_MASK);
            unsigned char c = (unsigned char)(g_virt[idx] & A_CHARTEXT);
            putchar(c ? c : ' ');
            g_phys[idx] = g_virt[idx];
            last_y = y;
            last_x = x + 1;
            if (last_x >= g_cols) {
                last_y = -1; /* force a CUP next time we write on this row again */
            }
        }
    }
    if (stdscr) {
        printf("\033[%d;%dH", stdscr->cury + 1 + stdscr->begy, stdscr->curx + 1 + stdscr->begx);
    }
    fflush(stdout);
    return OK;
}

int wrefresh(WINDOW *win) {
    if (wnoutrefresh(win) != OK) {
        return ERR;
    }
    return doupdate();
}

int refresh(void) {
    return wrefresh(stdscr);
}

int touchwin(WINDOW *win) {
    (void)win;
    return OK;
}

static int decode_escape(void) {
    char c1, c2;
    if (read(0, &c1, 1) != 1) {
        return 27;
    }
    if (c1 != '[' && c1 != 'O') {
        return 27;
    }
    if (read(0, &c2, 1) != 1) {
        return 27;
    }
    switch (c2) {
    case 'A': return KEY_UP;
    case 'B': return KEY_DOWN;
    case 'C': return KEY_RIGHT;
    case 'D': return KEY_LEFT;
    case 'H': return KEY_HOME;
    case 'F': return KEY_END;
    case '3': {
        char c3;
        if (read(0, &c3, 1) == 1 && c3 == '~') {
            return KEY_DC;
        }
        return 27;
    }
    default:
        return 27;
    }
}

int wgetch(WINDOW *win) {
    unsigned char c;
    ssize_t n = read(0, &c, 1);
    if (n <= 0) {
        return ERR;
    }
    if (c == 127) {
        return KEY_BACKSPACE;
    }
    if (win && win->keypad_on && c == 27) {
        return decode_escape();
    }
    return c;
}

int tc_win_height(WINDOW *win) { return win ? win->maxy : 0; }
int tc_win_width(WINDOW *win) { return win ? win->maxx : 0; }
int tc_win_begy(WINDOW *win) { return win ? win->begy : 0; }
int tc_win_begx(WINDOW *win) { return win ? win->begx : 0; }
int tc_win_cury(WINDOW *win) { return win ? win->cury : 0; }
int tc_win_curx(WINDOW *win) { return win ? win->curx : 0; }
