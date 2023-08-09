#include "term.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>
#include <stdbool.h>

void ncurses_init(bool color_flag)
{
    initscr();
    clear();
    noecho();    // Input characters aren't echoed
    cbreak();    // Disable line buffering
    curs_set(0); // Make cursor invisible
    timeout(0);  // Non-blocking read for getch

    // Initialize colors
    if (color_flag && has_colors())
    {
        start_color();
        use_default_colors(); // Use terminal colors (fg and bg for pair 0)

        // Colors with default backgrounds
        init_pair(1, COLOR_BLACK,   -1);
        init_pair(2, COLOR_RED,     -1);
        init_pair(3, COLOR_GREEN,   -1);
        init_pair(4, COLOR_YELLOW,  -1);
        init_pair(5, COLOR_BLUE,    -1);
        init_pair(6, COLOR_MAGENTA, -1);
        init_pair(7, COLOR_CYAN,    -1);
        init_pair(8, COLOR_WHITE,   -1);
    }
}

void ncurses_kill()
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
#endif  // WINDOWS
}


bool stdout_directed_to_console()
{
#ifdef WINDOWS
    // Hacky way to check if stdout is directed to a console
    // https://stackoverflow.com/questions/2087775/how-do-i-detect-when-output-is-being-redirected
    fpost_t pos;
    fgetpos(stdout, &pos);
    return (pos == -1);
#else
    return isatty(fileno(stdout));
#endif  // WINDOWS
}

float get_cell_aspect_ratio()
{
    float default_height = 2;

    // Attempt to get aspect ratio only if stdout writing to console
    if (stdout_directed_to_console())
    {
#ifdef WINDOWS
        static const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_FONT_INFO cfi;
        GetCurrentConsoleFont(handle, false, &cfi);
        float cell_width = cfi.dwFontSize.X;
        float cell_height = cfi.dwFontSize.Y;

        return cell_height / cell_width;
#else
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        // In case we can't get pixel size of terminal (inconsistent support)
        if (ws.ws_ypixel == 0 || ws.ws_xpixel == 0)
        {
            return default_height;
        }

        float cell_height = (float) ws.ws_ypixel / ws.ws_row;
        float cell_width = (float) ws.ws_xpixel / ws.ws_col;

        return cell_height / cell_width;
#endif  // WINDOWS
    }

    return default_height;
}
