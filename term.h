/* Terminal and ncurses common functions and utilities.
 */

#ifndef TERM_H
#define TERM_H

#include <ncurses.h>

/* Initialize ncurses
 */
void ncurses_init(bool color_flag);

/* Kill ncurses
 */
void ncurses_kill();


/* Resize window to square with largest possible area
 * aspect: cell aspect ratio (font height to width)
 */
void win_resize_square(WINDOW *win, float aspect);

/* Resize a window to full screen
 */
void win_resize_full(WINDOW *win);

/* Center window vertically and horizontally
 */
void win_position_center(WINDOW *win);


/* Get the number of rows and columns in the terminal buffer
 */
void term_size(int *y, int *x);

/* attempt to get the cell aspect ratio: cell height to width
 * i.e. "how many columns form the apparent height of a row"
 */
float get_cell_aspect_ratio();

#endif  // TERM_H