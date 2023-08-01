#include "drawing.h"

#include <ncurses.h>
#include <math.h>
#include <stdlib.h>

void draw_line_smooth(WINDOW *win, int ya, int xa, int yb, int xb)
{
    // The logic here is not particularly elegant or efficient

    int dy = yb - ya;
    int dx = xb - xa;

    // "Joint"/junction characters
    char *joint_a;
    char *joint_b;

    if (abs(dy) > abs(dx))
    {
        // No intelligence... just choose based on case
        if (dx > 0)
        {
            joint_a = dy > 0 ? "╰" : "╭";
            joint_b = dy > 0 ? "╮" : "╯";
        }
        else
        {
            joint_a = dy > 0 ? "╯" : "╮";
            joint_b = dy > 0 ? "╭" : "╰";
        }

        int y = 0;
        double x = 0.0;
        double nextX;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double) dx / abs(dy);

        while (abs(y) < abs(dy))
        {
            mvwaddstr(win, ya + y, xa + (int) x, "│");

            // Draw joint if we jump a column && we're not on the last cell 
            if ((int) (x + sx) != (int) x && xa + (int) x != xb)
            {
                mvwaddstr(win, ya + y, xa + (int) x, joint_a);
                mvwaddstr(win, ya + y, xa + (int)(x + sx), joint_b);
            }

            y += sy;
            x += sx;
        }
    }
    else
    {
        // No intelligence... just choose based on case
        if (dy > 0)
        {
            joint_a = dx > 0 ? "╮" : "╭";
            joint_b = dx > 0 ? "╰" : "╯";
        }
        else
        {
            joint_b = dx > 0 ? "╭" : "╮";
            joint_a = dx > 0 ? "╯" : "╰";
        }

        double y = 0.0;
        int x = 0;
        double nextY;

        // Step size
        double sy = (double) dy / abs(dx);
        int sx = (dx > 0) ? 1 : -1;

        while (abs(x) <= abs(dx))
        {
            mvwaddstr(win, ya + (int) y, xa + x, "─");

            // Draw joint if we jump a row && we're not on the last cell
            if ((int) (y + sy) != (int) y && ya + (int) y != yb)
            {
                mvwaddstr(win, ya + (int) y, xa + x, joint_a);
                mvwaddstr(win, ya + (int) (y + sy), xa + x, joint_b);
            }

            y += sy;
            x += sx;
        }
    }

    // Add circles at beginning and end of segment to "prettify"
    mvwaddstr(win, ya, xa, "◯");
    mvwaddstr(win, yb, xb, "◯");
}

enum fillType
{
    HORIZONTAL,
    VERTICAL,
    CORNER,
};

// Reference: https://dai.fmph.uniba.sk/upload/0/01/Ellipse.pdf

void print_chars_ellipse_ASCII(WINDOW *win, int center_y, int center_x,
                               int y, int x, int fill)
{
    switch (fill)
    {
        case CORNER:
            mvwaddch(win, center_y - y, center_x + x, '\\'); // Quad I
            mvwaddch(win, center_y - y, center_x - x, '/');  // Quad II
            mvwaddch(win, center_y + y, center_x - x, '\\'); // Quad III
            mvwaddch(win, center_y + y, center_x + x, '/');  // Quad IV
            break;

        case VERTICAL:
            mvwaddch(win, center_y - y, center_x + x, '|');
            mvwaddch(win, center_y - y, center_x - x, '|');
            mvwaddch(win, center_y + y, center_x - x, '|');
            mvwaddch(win, center_y + y, center_x + x, '|');
            break;

        case HORIZONTAL:
            mvwaddch(win, center_y - y, center_x + x, '-');
            mvwaddch(win, center_y - y, center_x - x, '-');
            mvwaddch(win, center_y + y, center_x - x, '-');
            mvwaddch(win, center_y + y, center_x + x, '-');
            break;
    }
}

void print_chars_ellipse_unicode(WINDOW *win, int center_y, int center_x,
                                 int y, int x, int fill)
{
    // TODO: def not correct
    switch (fill)
    {
        case CORNER:
            // Quad I
            mvwaddstr(win, center_y - y - 1, center_x + x,    "╮");
            mvwaddstr(win, center_y - y, center_x + x,        "╰");
            // Quad II
            mvwaddstr(win, center_y - y - 1, center_x - x,    "╭");
            mvwaddstr(win, center_y - y, center_x - x,        "╯");
            // Quad III
            mvwaddstr(win, center_y + y - 1, center_x - x,    "╮");
            mvwaddstr(win, center_y + y, center_x - x,        "╰");
            // Quad IV
            mvwaddstr(win, center_y + y - 1, center_x + x,    "╭");
            mvwaddstr(win, center_y + y, center_x + x,        "╯");
            break;

        case VERTICAL:
            mvwaddstr(win, center_y - y, center_x + x, "│");
            mvwaddstr(win, center_y - y, center_x - x, "│");
            mvwaddstr(win, center_y + y, center_x - x, "│");
            mvwaddstr(win, center_y + y, center_x + x, "│");
            break;

        case HORIZONTAL:
            mvwaddstr(win, center_y - y, center_x + x, "─");
            mvwaddstr(win, center_y - y, center_x - x, "─"); 
            mvwaddstr(win, center_y + y, center_x - x, "─");
            mvwaddstr(win, center_y + y, center_x + x, "─");
            break;
    }

    return;
}


int ellipse_error(int y, int x, int rad_y, int rad_x)
{
    (rad_x * rad_x + x * x) + (rad_y * rad_y + y * y) - (rad_x * rad_x * rad_y * rad_y);
}

void draw_ellipse(WINDOW *win, int center_y, int center_x,
                 int rad_y, int rad_x,
                 bool no_unicode)
{

    int y = 0;
    int x = rad_x;

    int y_next, x_next;

    // Point where slope = -1
    int magicY = sqrt(pow(rad_y, 4) / (rad_x * rad_x + rad_y * rad_y));

    // Print first part of first quadrant: slope is > -1
    while (y_next > magicY)
    {

        // If outside ellipse, move inward
        y_next = y + 1;
        x_next = (ellipse_error(y_next, x_next, rad_y, rad_x) > 0) ? x - 1 : x;

        bool corner = y_next > y && x_next < x;
        bool vertical = y_next > y && x_next == x;

        char fill = corner ? CORNER : 
                    vertical ? VERTICAL :
                    HORIZONTAL;

        if (no_unicode)
        {
            print_chars_ellipse_ASCII(win, center_y, center_x, y, x, fill);
        }
        else
        {
            print_chars_ellipse_unicode(win, center_y, center_x, y, x, fill);
        }

        y = y_next;
        x = x_next;
    }

    // Print second part of first quadrant: slope is < -1
    while (x_next > 0)
    {

        // If inside ellipse, move outward
        y_next = (ellipse_error(y_next, x_next, rad_y, rad_x) < 0) ? y + 1 : y;
        x_next = x - 1;

        bool corner = y_next > y && x_next < x;
        bool vertical = y_next > y && x_next == x;

        char fill = corner ? CORNER : 
                    vertical ? VERTICAL :
                    HORIZONTAL;

        if (no_unicode)
        {
            print_chars_ellipse_ASCII(win, center_y, center_x, y, x, fill);
        }
        else
        {
            print_chars_ellipse_unicode(win, center_y, center_x, y, x, fill);
        }

        y = y_next;
        x = x_next;
    }

    return;
}