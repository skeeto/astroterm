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

float get_cell_aspect_ratio()
{
    float default_height = 2.15;

#ifdef WINDOWS
    static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFO cfi;
    GetCurrentConsoleFont(handle, false, &cfi);
    float cell_width = cfi.dwFontSize.X;
    float cell_height = cfi.dwFontSize.Y;

    return cell_height / cell_width;
#else
    if (isatty(fileno(stdout)))
    {
        // Stdout is outputting to a terminal

        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        // in case we can't get pixel size of terminal
        if (ws.ws_ypixel == 0 || ws.ws_xpixel == 0)
        {
            return default_height;
        }

        float cell_height = (float)ws.ws_ypixel / ws.ws_row;
        float cell_width = (float)ws.ws_xpixel / ws.ws_col;

        return cell_height / cell_width;
    }
#endif

    return default_height;
}
