#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>

void term_init()
{
    initscr();
    clear();
    noecho();    // input characters aren't echoed
    cbreak();    // disable line buffering
    curs_set(0); // make cursor inisible
    timeout(0);  // non-blocking read for getch
}

void term_kill()
{
    endwin();
}

void term_size(int *y, int *x)
{
    #ifdef WINDOWS
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        *y = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *x = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    #else
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        *y = ws.ws_row;
        *x = ws.ws_col;
    #endif
}

void win_resize_square(WINDOW *win, float aspect)
{
    if (COLS < LINES * aspect)
    {
        wresize(win, COLS / aspect, COLS);
    }
    else
    {
        wresize(win, LINES, LINES * aspect);
    }
}

void win_resize_full(WINDOW *win)
{
    wresize(win, LINES, COLS);
}

void win_position_center(WINDOW *win)
{
    mvwin(win, (LINES - win->_maxy) / 2, (COLS - win->_maxx) / 2);
}
