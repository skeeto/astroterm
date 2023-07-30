#include <ncurses.h>
#include <math.h>
#include <stdlib.h>

enum fillType
{
    HORIZONTAL,
    VERTICAL,
    CORNER,
};

//-------------------------------Line Drawing---------------------------------//

// void printCharsLineASCII(int y, int x, fillType fill, WINDOW *win)
// {
    
// }

// void lineError(int y, int x, int yA, int xA, int yB, int xB)
// {
//     (x - xA) * (yB - yA) - (y - yA) * (xB - xA);
// }

// void drawLineASCII(int colA, int rowA, int colB, int rowB, WINDOW *win)
// {
//     int rows = abs(rowB - rowA);
//     int cols = abs(colB - colA);

//     int y = 0, x = 0;
//     int yNext, xNext;

//     int directionY = (rowA > rowB) ? -1 : 1;
//     int directionX = (colA > colB) ? -1 : 1;

//     char primaryChar = (rows > cols) ? '|' : '-';
//     char cornerChar = ((rowB - rowA) ^ (colB - colA) > 0) ?  '/' : '\\';
//     // Use bitwise XOR to efficiently determine sign of slope

//     if (rows > cols)
//     {
//         // |slope| > 1
//         while (abs(y) <= rows / 2 + 1) // Ensure we hit the midpoint
//         {
//             yNext = y + directionY;
//             xNext = (lineError(y, x, 0, 0, yEnd, xEnd < 0)) ? x + directionX : x;

//             bool corner = (xNext != x);

//             if (corner)
//             {
//                 mvwaddch(win, rowA + y, colA + x, cornerChar);
//                 mvwaddch(win, rowB - y, colB - x, cornerChar);
//             }
//             else
//             {
//                 mvwaddch(win, rowA + y, colA + x, primaryChar);
//                 mvwaddch(win, rowB - y, colB - x, primaryChar);
//             }

//             y = yNext;
//             x = xNext;
//         }
//     }
//     else
//     {
//         // |slope| <= 1
//         while (abs(x) < cols / 2 + 1) // Ensure we hit the midpoint
//         {
//             yNext = (lineError(y, x, 0, 0, yEnd, xEnd < 0)) ? y + directionY : y;
//             xNext = x + xDirection;

//             bool corner = (yNext != y);

//             if (corner)
//             {
//                 mvwaddch(win, rowA + y, colA + x, cornerChar);
//                 mvwaddch(win, rowB - y, colB - x, cornerChar);
//             }
//             else
//             {
//                 mvwaddch(win, rowA + y, colA + x, primaryChar);
//                 mvwaddch(win, rowB - y, colB - x, primaryChar);
//             }

//             y = yNext;
//             x = xNext;
//         }
//     }
// }

void drawLineBasic(WINDOW *win, int ya, int xa, int yb, int xb, bool no_unicode)
{
    int dy = yb - ya;
    int dx = xb - xa;

    // Determine slope of line
    char cornerChar = (dy ^ dx) > 0 ? '\\' : '/';

    char *cornerStringA = (dy ^ dx) > 0 ? "╮" : "╭";
    char *cornerStringB = (dy ^ dx) > 0 ? "╰" : "╯";

    if (abs(dy) > abs(dx))
    {
        int y = 0;
        double x = 0;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double)dx / abs(dy);

        double nextX;

        // Rounding ensures symmetry
        while (y != dy)
        {
            nextX = x + sx;

            if (no_unicode)
            {
                mvwaddch(win, ya + y, round(xa + x), round(nextX) != round(x) ? cornerChar : '|');
            }
            else
            {
                mvwaddstr(win, ya + y, round(xa + x), round(nextX) != round(x) ? cornerStringA : "│");
            }

            x = nextX;
            y += sy;
        }
    }
    else 
    {
        double y = 0;
        int x = 0;

        // Step size
        int sx = (dx > 0) ? 1 : -1;
        double sy = (double)dy / abs(dx);

        // Round (dx / 2) away from 0
        int midX = dx + (dx > 0 ? 1 : 0) >> 1;
        double nextY;

        // Rounding ensures symmetry
        while (x != dx)
        {
            nextY = y + sy;

            if (no_unicode)
            {
                mvwaddch(win, round(ya + y), xa + x, round(nextY) != round(y) ? cornerChar : '-');
            }
            else
            {
                mvwaddstr(win, round(ya + y), xa + x, round(nextY) != round(y) ? cornerStringA : "─");
                mvwaddstr(win, round(ya + y + sy), xa + x, round(nextY) != round(y) ? cornerStringB : "─");
            }

            y = nextY;
            x += sx;
        }
    }
}

void drawLine(WINDOW *win, int ya, int xa, int yb, int xb, bool no_unicode)
{
    int dy = yb - ya;
    int dx = xb - xa;

    // Determine slope of line
    char cornerChar = (dy ^ dx) > 0 ? '\\' : '/';

    char *cornerStringA = (dy ^ dx) > 0 ? "╮" : "╭";
    char *cornerStringB = (dy ^ dx) > 0 ? "╰" : "╯";

    if (abs(dy) > abs(dx))
    {

        int y = 0;
        double x = 0;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double) dx / abs(dy);

        // Round (dy / 2) away from 0
        int midY = dy + (dy > 0 ? 1 : 0) >> 1;
        double nextX;

        // Rounding ensures symmetry
        while (y != midY + sy)
        {
            nextX = x + sx;

            if (no_unicode)
            {
                mvwaddch(win, ya + y, round(xa + x), round(nextX) != round(x) ? cornerChar : '|');
                mvwaddch(win, yb - y, round(xb - x), round(nextX) != round(x) ? cornerChar : '|');
            }
            else 
            {
                mvwaddstr(win, ya + y, round(xa + x), round(nextX) != round(x) ? cornerStringA : "│");
                mvwaddstr(win, yb - y, round(xb - x), round(nextX) != round(x) ? cornerStringA : "│");
            }
            
            x = nextX; 
            y += sy;
        }
    }
    else
    {

        double y = 0;
        int x = 0;

        // Step size
        int sx = (dx > 0) ? 1 : -1;
        double sy = (double) dy / abs(dx);

        // Round (dx / 2) away from 0
        int midX = dx + (dx > 0 ? 1 : 0) >> 1;
        double nextY;

        // Rounding ensures symmetry
        while (x != midX)
        {
            nextY = y + sy;

            if (no_unicode)
            {
                mvwaddch(win, round(ya + y), xa + x, round(nextY) != round(y) ? cornerChar : '-');
                mvwaddch(win, round(yb - y), xb - x, round(nextY) != round(y) ? cornerChar : '-');
            }
            else
            {
                mvwaddstr(win, round(ya + y), xa + x, round(nextY) != round(y) ? cornerStringB : "─");
                mvwaddstr(win, round(yb - y), xb - x, round(nextY) != round(y) ? cornerStringA : "─");

                mvwaddstr(win, round(ya + y + sy), xa + x, round(nextY) != round(y) ? cornerStringA : "─");
                mvwaddstr(win, round(yb - y - sy), xb - x, round(nextY) != round(y) ? cornerStringB : "─");
            }

            y = nextY;
            x += sx;
        }
    }
    
    return;
}

void drawLineTest(WINDOW *win, int ya, int xa, int yb, int xb, bool no_unicode)
{
    int dy = yb - ya;
    int dx = xb - xa;

    char *cornerStringA;
    char *cornerStringB;

    if (abs(dy) > abs(dx))
    {
        if (dy ^ dx > 0)
        {
            cornerStringA = dy > 0 ? "╰" : "╮";
            cornerStringB = dy > 0 ? "╮" : "╰";
        }
        else
        {
            cornerStringA = dy > 0 ? "╯" : "╭";
            cornerStringB = dy > 0 ? "╭" : "╯";
        }

        int y = 0;
        double x = 0.0;
        double nextX;

        // Step size
        int sy = (dy > 0) ? 1 : -1;
        double sx = (double) dx / abs(dy);

        while (abs(y) <= abs(dy))
        {
            mvwaddstr(win, ya + y, xa + round(x), "│");

            if (round(x + sx) != round(x))
            {
                // Draw joint
                mvwaddstr(win, ya + y + sy, xa + round(x), cornerStringA);
                y += sy;
                x += sx;
                mvwaddstr(win, ya + y, xa + round(x), cornerStringB);
            }

            y += sy;
            x += sx;
        }
    }
    else
    {
        if (dy ^ dx > 0)
        {
            cornerStringA = dx > 0 ? "╯" : "╭";
            cornerStringB = dx > 0 ? "╭" : "╯";
        }
        else
        {
            cornerStringA = dx > 0 ? "╮" : "╰";
            cornerStringB = dx > 0 ? "╰" : "╮";
        }

        double y = 0.0;
        int x = 0;
        double nextY;

        // Step size
        double sy = (double) dy / abs(dx);
        int sx = (dx > 0) ? 1 : -1;

        while (abs(x) <= abs(dx))
        {
            mvwaddstr(win, ya + round(y), xa + x, "─");

            if (round(y + sy) != round(y))
            {
                // Draw joint
                mvwaddstr(win, ya + round(y), xa + x + sx, cornerStringA);
                y += sy;
                x += sx;
                mvwaddstr(win, ya + round(y), xa + x, cornerStringB);
            }

            y += sy;
            x += sx;
        }
    }
}

//------------------------------Ellipse Drawing-------------------------------//

// Reference: https://dai.fmph.uniba.sk/upload/0/01/Ellipse.pdf

void printCharsEllipseASCII(WINDOW *win, int centerRow, int centerCol,
                            int y, int x, int fill)
{
    switch (fill)
    {
        case CORNER:
            mvwaddch(win, centerRow - y, centerCol + x, '\\'); // Quad I
            mvwaddch(win, centerRow - y, centerCol - x, '/');  // Quad II
            mvwaddch(win, centerRow + y, centerCol - x, '\\'); // Quad III
            mvwaddch(win, centerRow + y, centerCol + x, '/');  // Quad IV
            break;

        case VERTICAL:
            mvwaddch(win, centerRow - y, centerCol + x, '|');
            mvwaddch(win, centerRow - y, centerCol - x, '|');
            mvwaddch(win, centerRow + y, centerCol - x, '|');
            mvwaddch(win, centerRow + y, centerCol + x, '|');
            break;

        case HORIZONTAL:
            mvwaddch(win, centerRow - y, centerCol + x, '-');
            mvwaddch(win, centerRow - y, centerCol - x, '-');
            mvwaddch(win, centerRow + y, centerCol - x, '-');
            mvwaddch(win, centerRow + y, centerCol + x, '-');
            break;
    }
}

void printCharsEllipseUnicode(WINDOW *win, int centerRow, int centerCol,
                              int y, int x, int fill)
{
    // TODO: def not correct
    switch (fill)
    {
        case CORNER:
            // Quad I
            mvwaddstr(win, centerRow - y - 1, centerCol + x,    "╮");
            mvwaddstr(win, centerRow - y, centerCol + x,        "╰");
            // Quad II
            mvwaddstr(win, centerRow - y - 1, centerCol - x,    "╭");
            mvwaddstr(win, centerRow - y, centerCol - x,        "╯");
            // Quad III
            mvwaddstr(win, centerRow + y - 1, centerCol - x,    "╮");
            mvwaddstr(win, centerRow + y, centerCol - x,        "╰");
            // Quad IV
            mvwaddstr(win, centerRow + y - 1, centerCol + x,    "╭");
            mvwaddstr(win, centerRow + y, centerCol + x,        "╯");
            break;

        case VERTICAL:
            mvwaddstr(win, centerRow - y, centerCol + x, "│");
            mvwaddstr(win, centerRow - y, centerCol - x, "│");
            mvwaddstr(win, centerRow + y, centerCol - x, "│");
            mvwaddstr(win, centerRow + y, centerCol + x, "│");
            break;

        case HORIZONTAL:
            mvwaddstr(win, centerRow - y, centerCol + x, "─");
            mvwaddstr(win, centerRow - y, centerCol - x, "─"); 
            mvwaddstr(win, centerRow + y, centerCol - x, "─");
            mvwaddstr(win, centerRow + y, centerCol + x, "─");
            break;
    }

    return;
}


int ellipseError(int y, int x, int yRad, int xRad)
{
    (xRad * xRad + x * x) + (yRad * yRad + y * y) - (xRad * xRad * yRad * yRad);
}

void drawEllipse(WINDOW *win, int centerRow, int centerCol,
                 int yRad, int xRad,
                 bool no_unicode)
{

    int y = 0;
    int x = xRad;

    int yNext, xNext;

    // Point where slope = -1
    int magicY = sqrt(pow(yRad, 4) / (xRad * xRad + yRad * yRad));

    // Print first part of first quadrant: slope is > -1
    while (yNext > magicY)
    {

        // If outside ellipse, move inward
        yNext = y + 1;
        xNext = (ellipseError(yNext, xNext, yRad, xRad) > 0) ? x - 1 : x;

        bool corner = yNext > y && xNext < x;
        bool vertical = yNext > y && xNext == x;

        char fill = corner ? CORNER : 
                    vertical ? VERTICAL :
                    HORIZONTAL;

        if (no_unicode)
        {
            printCharsEllipseASCII(win, centerRow, centerCol, y, x, fill);
        }
        else
        {
            printCharsEllipseUnicode(win, centerRow, centerCol, y, x, fill);
        }

        y = yNext;
        x = xNext;
    }

    // Print second part of first quadrant: slope is < -1
    while (xNext > 0)
    {

        // If inside ellipse, move outward
        yNext = (ellipseError(yNext, xNext, yRad, xRad) < 0) ? y + 1 : y;
        xNext = x - 1;

        bool corner = yNext > y && xNext < x;
        bool vertical = yNext > y && xNext == x;

        char fill = corner ? CORNER : 
                    vertical ? VERTICAL :
                    HORIZONTAL;

        if (no_unicode)
        {
            printCharsEllipseASCII(win, centerRow, centerCol, y, x, fill);
        }
        else
        {
            printCharsEllipseUnicode(win, centerRow, centerCol, y, x, fill);
        }

        y = yNext;
        x = xNext;
    }

    return;
}