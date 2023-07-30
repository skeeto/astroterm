/* ASCII and Unicode ncruses rendering functions. These functions aim to provide
 * a balance of performance, readability, and style of the resulting render, with
 * more emphasis placed on the latter two objectives. Here, we forgo many of the
 * micro-optimizations (e.g. precomputing frequently used values) of the
 * inspiring/underlying algorithms, as the runtime of these functions will
 * largely be dominated by slow nature of drawing characters to a terminal, as
 * opposed to CPU arithmetic.
 *
 * Functions receive integer coordinates representing rows and columns on the
 * terminal screen: any calculation needed to adjust for the aspect ratio of
 * cells should be done before hand. Within each function, cell coordinates are
 * translated to conform to a normal cartesian grid. Points on this grid are
 * represented as `y` and `x` and are only translated to their respective `row`
 * and `column` on the terminal when they are pushed to the screen buffer.
 *
 * IMPORTANT:   using Unicode-designated functions requires UTF-8 encoding
 *              for proper results
 *
 * TODO:        function naming argument: verb - noun - adjectives
 *              (postpositive adjectives)
 */

#include <ncurses.h>
#include <stdbool.h>

#ifndef DRAWING_H
#define DRAWING_H

/* Draw a line from point A to point B. If pointA == pointB, a random
 * "line character" will be placed at that coordinate, i.e. don't do this
 */
void drawLine(WINDOW *win, int colA, int rowA, int colB, int rowB, bool no_unicode);
void drawLineTest(WINDOW *win, int colA, int rowA, int colB, int rowB, bool no_unicode);

/* Draw an ellipse. By taking advantage of knowing the cell aspect ratio,
 * this function can generate an "apparent" circle.
 */
void drawEllipse(WINDOW *win, int centerRow, int centerCol,
                 int radiusY, int radiusX, bool no_unicode);

#endif