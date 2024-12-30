#include "term.h"

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void ncurses_init(bool color)
{
    initscr();
    clear();
    noecho();    // Input characters aren't echoed
    cbreak();    // Disable line buffering
    curs_set(0); // Make cursor invisible
    timeout(0);  // Non-blocking read for getch

    // Initialize colors
    if (color)
    {
        if (!has_colors())
        {
            printf("Your terminal does not support colors");
            exit(EXIT_FAILURE);
        }

        start_color();
        use_default_colors(); // Use terminal colors (fg and bg for pair 0)

        // Colors with default backgrounds
        init_pair(1, COLOR_BLACK, -1);
        init_pair(2, COLOR_RED, -1);
        init_pair(3, COLOR_GREEN, -1);
        init_pair(4, COLOR_YELLOW, -1);
        init_pair(5, COLOR_BLUE, -1);
        init_pair(6, COLOR_MAGENTA, -1);
        init_pair(7, COLOR_CYAN, -1);
        init_pair(8, COLOR_WHITE, -1);
    }
}

void ncurses_kill(void)
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

void wrectangle(WINDOW *win, int ya, int xa, int yb, int xb)
{
    mvwhline(win, ya, xa, 0, xb - xa);
    mvwhline(win, yb, xa, 0, xb - xa);
    mvwvline(win, ya, xa, 0, yb - ya);
    mvwvline(win, ya, xb, 0, yb - ya);
    mvwaddch(win, ya, xa, ACS_ULCORNER);
    mvwaddch(win, yb, xa, ACS_LLCORNER);
    mvwaddch(win, ya, xb, ACS_URCORNER);
    mvwaddch(win, yb, xb, ACS_LRCORNER);
}

void win_resize_full(WINDOW *win)
{
    wresize(win, LINES, COLS);
}

void win_position_center(WINDOW *win)
{
    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int center_y = (LINES - maxy) / 2;
    int center_x = (COLS - maxx) / 2;

    mvwin(win, center_y, center_x);
}

void term_size(int *y, int *x)
{
#if defined(_WIN32)

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *y = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *x = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

#else

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *y = (int)ws.ws_row;
    *x = (int)ws.ws_col;

#endif // _WIN32
}

bool stdout_directed_to_console(void)
{
#if defined(_WIN32)

    // Hacky way to check if stdout is directed to a console
    // https://stackoverflow.com/questions/2087775/how-do-i-detect-when-output-is-being-redirected
    fpost_t pos;
    fgetpos(stdout, &pos);
    return (pos == -1);

#else

    return (isatty(fileno(stdout)) != 0);

#endif // _WIN32
}

float get_cell_aspect_ratio(void)
{
    float default_height = 2;

    // Attempt to get aspect ratio only if stdout writing to console
    if (stdout_directed_to_console())
    {
#if defined(_WIN32)

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

        float cell_height = (float)ws.ws_ypixel / ws.ws_row;
        float cell_width = (float)ws.ws_xpixel / ws.ws_col;

        return cell_height / cell_width;

#endif // _WIN32
    }

    return default_height;
}

void mvwaddstr_truncate(WINDOW *win, int y, int x, const char *str)
{
    // Remaining space on the current line
    int max_x = getmaxx(win);
    int space_left = max_x - x;

    // Don't write beyond the line
    if (space_left > 0)
    {
        // Truncate if necessary
        char truncated[space_left + 1]; // +1 for the null terminator
        strncpy(truncated, str, space_left);
        truncated[space_left] = '\0';
        mvwaddstr(win, y, x, truncated);
    }
}
