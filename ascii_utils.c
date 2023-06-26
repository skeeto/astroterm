#include <stdbool.h>
#include <ncurses.h>

/* draw an ASCII line from point A to point B. If pointA == pointB, a random
 * "line character" will be placed at that coordinate, i.e. don't do this
 */
void drawLineASCII(int colA, int rowA, int colB, int rowB, WINDOW *win)
{
    enum direction
    {
        VERTICAL,
        HORIZONTAL
    };

    enum direction primaryDirection;

    int rows = abs(rowB - rowA);
    int cols = abs(colB - colA);

    int rowDirection = (rowB > rowA) ? 1 : -1;
    int colDirection = (colB > colA) ? 1 : -1;

    /* determine the primary "direction" we are moving in
     * this is equivalent to the direction which requires more movement
     * this is important as the algorithm will always move one unit
     * in the primary direction, and some fraction of a unit in the other,
     * ensuring efficiency and coverage of all required grid spaces.
     * efficient enough for our purposes
     */
    primaryDirection = (rows > cols) ? VERTICAL : HORIZONTAL;

    char primaryChar;
    char cornerChar;

    // TODO: simplify character selection
    switch (primaryDirection)
    {
        case VERTICAL:
            primaryChar = '|';
            break;
        case HORIZONTAL:
            primaryChar = '-';
            break;
    }

    // we check rowDirection < 0 as we are working in graphical coords, i.e. y-axis is flipped
    if (rowDirection < 0)
    {
        cornerChar = (colDirection > 0) ? '/' : '\\';
    }
    else
    {
        cornerChar = (colDirection > 0) ? '\\' : '/';
    }

    // quantity to move in each direction per step
    float rowStep, colStep;

    // total number of steps in primary direction
    int steps;

    switch (primaryDirection)
    {
        case VERTICAL:
            rowStep = (float)rows / rows * rowDirection;
            colStep = (float)cols / rows * colDirection;
            steps = rows;
            break;

        case HORIZONTAL:
            rowStep = (float)rows / cols * rowDirection;
            colStep = (float)cols / cols * colDirection;
            steps = cols;
            break;
    }

    // start at current point
    int row = rowA
    int col = colA;

    for (int t = 0; t <= steps; t++)
    {

        // calculate position of *next* step
        int nextRow = round((t + 1) * rowStep) + rowA;
        int nextCol = round((t + 1) * colStep) + colA;

        // determine if corner is needed
        bool cornerNeeded;

        switch (primaryDirection)
        {
            case VERTICAL:
                cornerNeeded = nextCol != col;
                break;

            case HORIZONTAL:
                cornerNeeded = nextRow != row;
                break;
        }

        mvwaddch(win, row, col, cornerNeeded ? cornerChar : primaryChar);

        row = nextRow;
        col = nextCol;
    }

    return;
}