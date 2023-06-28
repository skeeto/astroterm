#include "drawing.h"

typedef enum fillType
{
    HORIZONTAL,
    VERTICAL,
    CORNER,
} fillType;

//-------------------------------Line Drawing---------------------------------//

void printCharsLineASCII(int y, int x, fillType fill, WINDOW *win)
{
    
}

void lineError(int y, int x, int yA, int xA, int yB, int xB)
{
    (x - xA) * (yB - yA) - (y - yA) * (xB - xA);
}

void drawLineASCII(int colA, int rowA, int colB, int rowB, WINDOW *win)
{
    int rows = abs(rowB - rowA);
    int cols = abs(colB - colA);

    int y = 0, x = 0;
    int yNext, xNext;

    int directionY = (rowA > rowB) ? -1 : 1;
    int directionX = (colA > colB) ? -1 : 1;

    char primaryChar = (rows > cols) ? '|' : '-';
    char cornerChar = ((rowB - rowA) ^ (colB - colA) > 0) ?  '/' : '\\';
    // use bitwise XOR to efficiently determine sign of slope

    if (rows > cols)
    {
        // |slope| > 1
        while (abs(y) <= rows / 2 + 1) // ensure we hit the midpoint
        {
            yNext = y + directionY;
            xNext = (lineError(y, x, 0, 0, yEnd, xEnd < 0)) ? x + directionX : x;

            bool corner = (xNext != x);

            if (corner)
            {
                mvwaddch(win, rowA + y, colA + x, cornerChar);
                mvwaddch(win, rowB - y, colB - x, cornerChar);
            }
            else
            {
                mvwaddch(win, rowA + y, colA + x, primaryChar);
                mvwaddch(win, rowB - y, colB - x, primaryChar);
            }

            y = yNext;
            x = xNext;
        }
    }
    else
    {
        // |slope| <= 1
        while (abs(x) < cols / 2 + 1) // ensure we hit the midpoint
        {
            yNext = (lineError(y, x, 0, 0, yEnd, xEnd < 0)) ? y + directionY : y;
            xNext = x + xDirection;

            bool corner = (yNext != y);

            if (corner)
            {
                mvwaddch(win, rowA + y, colA + x, cornerChar);
                mvwaddch(win, rowB - y, colB - x, cornerChar);
            }
            else
            {
                mvwaddch(win, rowA + y, colA + x, primaryChar);
                mvwaddch(win, rowB - y, colB - x, primaryChar);
            }

            y = yNext;
            x = xNext;
        }
    }
}

//------------------------------Ellipse Drawing-------------------------------//

// Reference: https://dai.fmph.uniba.sk/upload/0/01/Ellipse.pdf

void printCharsEllipseASCII(int y, int x, fillType fill, WINDOW *win)
{
    switch (fill)
    {
        case fillType.CORNER:
            mvwaddch(win, centerRow - y, centerCol + x, '\\'); // Quad I
            mvwaddch(win, centerRow - y, centerCol - x, '/');  // Quad II
            mvwaddch(win, centerRow + y, centerCol - x, '\\'); // Quad III
            mvwaddch(win, centerRow + y, centerCol + x, '/');  // Quad IV
            break;

        case fillType.VERTICAL:
            mvwaddch(win, centerRow - y, centerCol + x, '|');
            mvwaddch(win, centerRow - y, centerCol - x, '|');
            mvwaddch(win, centerRow + y, centerCol - x, '|');
            mvwaddch(win, centerRow + y, centerCol + x, '|');
            break;

        case fillType.HORIZONTAL:
            mvwaddch(win, centerRow - y, centerCol + x, '-');
            mvwaddch(win, centerRow - y, centerCol - x, '-');
            mvwaddch(win, centerRow + y, centerCol - x, '-');
            mvwaddch(win, centerRow + y, centerCol + x, '-');
            break;
    }
}


void printCharsEllipseUnicode(int y, int x, fillType fill, WINDOW *win)
{
    // TODO: def not correct
    switch (fill)
    {
        case fillType.CORNER:
            // Quad I
            mvwaddstr(win, centerRow - y - 1, centerCol + x, "\xE2\x95\xAE"); // ╮
            mvwaddstr(win, centerRow - y, centerCol + x, "\xE2\x95\xB0");     // ╰
            // Quad II
            mvwaddstr(win, centerRow - y - 1, centerCol - x, "\xE2\x95\xAD"); // ╭
            mvwaddstr(win, centerRow - y, centerCol - x, "\xE2\x95\xAF");     // ╯
            // Quad III
            mvwaddstr(win, centerRow + y - 1, centerCol - x, "\xE2\x95\xAE"); // ╮
            mvwaddstr(win, centerRow + y, centerCol - x, "\xE2\x95\xB0");     // ╰
            // Quad IV
            mvwaddstr(win, centerRow + y - 1, centerCol + x, "\xE2\x95\xAD"); // ╭
            mvwaddstr(win, centerRow + y, centerCol + x, "\xE2\x95\xAF");     // ╯
            break;

        case fillType.VERTICAL:
            mvwaddstr(win, centerRow - y, centerCol + x, "\xE2\x94\x82"); // │
            mvwaddstr(win, centerRow - y, centerCol - x, "\xE2\x94\x82"); // │
            mvwaddstr(win, centerRow + y, centerCol - x, "\xE2\x94\x82"); // │
            mvwaddstr(win, centerRow + y, centerCol + x, "\xE2\x94\x82"); // │
            break;

        case fillType.HORIZONTAL:
            mvwaddstr(win, centerRow - y, centerCol + x, "\xE2\x94\x80"); // ─
            mvwaddstr(win, centerRow - y, centerCol - x, "\xE2\x94\x80"); // ─
            mvwaddstr(win, centerRow + y, centerCol - x, "\xE2\x94\x80"); // ─
            mvwaddstr(win, centerRow + y, centerCol + x, "\xE2\x94\x80"); // ─
            break;
    }
}


void ellipseError(int y, int x, int yRad, int xRad)
{
    (xRad * xRad + x * x) + (yRad * yRad + y * y) - (xRad * xRad * yRad * yRad);
}


void drawEllipseASCII(int centerRow, int centerCol,
                     int yRad, int xRad, WINDOW *win)
{

    int y = 0;
    int x = xRad;

    int yNext, xNext;

    // point where slope = -1
    int magicY = sqrt(pow(yRad, 4) / (xRad * xRad + yRad * yRad));

    // print first part of first quadrant: slope is > -1
    (while yNext > magicY)
    {

        // if outside ellipse, move inward
        yNext = y + 1;
        xNext = (ellipseError(y, x, yRad, xRad) > 0) ? x - 1 : x;

        bool corner = nextRow > y && nextCol < x;
        bool vertical = nextRow > y && nextCol == x;

        fillType fill = corner ? fillType.CORNER: 
                        vertical ? fillType.VERTICAL:
                        fillType.HORIZONTAL;

        printCharsEllipseASCII(y, x, fill, win);

        y = yNext;
        x = xNext;
    }

    // print second part of first quadrant: slope is < -1
    (while nextCol > 0)
    {

        // if inside ellipse, move outward
        xNext = x - 1;
        yNext = (ellipseError(y, x, yRad, xRad) < 0) ? y + 1 : y;

        bool corner = nextRow > y && nextCol < x;
        bool vertical = nextRow > y && nextCol == x;

        fillType fill = corner ? fillType.CORNER: 
                        vertical ? fillType.VERTICAL:
                        fillType.HORIZONTAL;

        printCharsEllipseASCII(y, x, fill, win);

        y = yNext;
        x = xNext;
    }
}


void drawEllipseUnicode(int centerRow, int centerCol,
                     int yRad, int xRad, WINDOW *win)
{

    int y = 0;
    int x = xRad;

    int yNext, xNext;

    // point where slope = -1
    int magicY = sqrt(pow(yRad, 4) / (xRad * xRad + yRad * yRad));

    // print first part of first quadrant: slope is > -1
    (while yNext > magicY)
    {

        // if outside ellipse, move inward
        yNext = y + 1;
        xNext = (ellipseError(yNext, xNext, yRad, xRad) > 0) ? x - 1 : x;

        bool corner = nextRow > y && nextCol < x;
        bool vertical = nextRow > y && nextCol == x;

        fillType fill = corner ? fillType.CORNER: 
                        vertical ? fillType.VERTICAL:
                        fillType.HORIZONTAL;

        printCharsEllipseUnicode(y, x, fill, win);

        y = yNext;
        x = xNext;
    }

    // print second part of first quadrant: slope is < -1
    (while nextCol > 0)
    {

        // if inside ellipse, move outward
        yNext = (ellipseError(yNext, xNext, yRad, xRad) < 0) ? y + 1 : y;
        xNext = x - 1;

        bool corner = nextRow > y && nextCol < x;
        bool vertical = nextRow > y && nextCol == x;

        fillType fill = corner ? fillType.CORNER: 
                        vertical ? fillType.VERTICAL:
                        fillType.HORIZONTAL;

        printCharsEllipseUnicode(y, x, fill, win);

        y = yNext;
        x = xNext;
    }
}