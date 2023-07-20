#include <ncurses.h>

#ifndef TERM_UTILS_H
#define TERM_UTILS_H

// Initialize ncurses
void term_init();
// Kill ncurses
void term_kill();

// Resize window to square with largest possible area
// aspect: cell aspect ratio (font height to width)
void win_resize_square(WINDOW *win, float aspect);
// Resize a window to full screen
void win_resize_full(WINDOW *win);
// Center window vertically and horizontally
void win_position_center(WINDOW *win);

// Get the number of rows and columns in the terminal buffer
void term_size(int *y, int *x);

#endif