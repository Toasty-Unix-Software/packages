#include "tcurses.h"

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLUE);

    box(stdscr, 0, 0);
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, 2, "tcurses demo - lightweight POSIX curses for TUS");
    attroff(COLOR_PAIR(1) | A_BOLD);
    mvprintw(3, 2, "LINES=%d COLS=%d", LINES, COLS);
    mvprintw(5, 2, "press arrow keys to move the marker, q to quit");
    refresh();

    int y = 8, x = 4;
    mvaddch(y, x, '*');
    refresh();

    int ch;
    while ((ch = getch()) != 'q') {
        mvaddch(y, x, ' ');
        switch (ch) {
        case KEY_UP: y--; break;
        case KEY_DOWN: y++; break;
        case KEY_LEFT: x--; break;
        case KEY_RIGHT: x++; break;
        default: break;
        }
        if (y < 7) y = 7;
        if (y > LINES - 2) y = LINES - 2;
        if (x < 1) x = 1;
        if (x > COLS - 2) x = COLS - 2;
        mvaddch(y, x, '*');
        mvprintw(6, 2, "key=%d y=%d x=%d      ", ch, y, x);
        refresh();
    }

    endwin();
    return 0;
}
