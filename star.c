#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>

#include "bin_utils.c"
#include "ascii_utils.c"


struct star
{
    float catalogNumber;
    float magnitude;
    double rightAscension;
    double declination;
};


void printStar(struct star *star)
{
    printf("ID#: %f\n", star->catalogNumber);
    printf("RA: %f\n", star->rightAscension);
    printf("DC: %f\n", star->declination);
    printf("Mag: %f\n", star->magnitude);
    return;
}


/* convert binary data entry to star struct
 */
struct star entryToStar(uint8_t *entry)
{
    struct star starData;

    // BSC5 Entry format
    starData.catalogNumber  = format_float32(0, entry);
    starData.rightAscension = format_double64(4, entry);
    starData.declination    = format_double64(12, entry);
    starData.magnitude      = (float) format_uint16(22, entry) / 100;

    return starData;
}


/* read BSC5 into memory for efficient access
 * slightly generalized to read other catalogs in SAOTDC binary format
 * TODO: requires more generalization if we're doing that
 */
struct star* readBSC5toMem(const char *filePath, int *returnNumStars)
{
    // Read header

    FILE *filePointer;
    filePointer = fopen(filePath, "rb");
    
    uint8_t headerBuffer[28]; // header defined as 28 bytes

    fread(headerBuffer, sizeof(headerBuffer), 1, filePointer);

    // We know BSC5 uses J2000 cords.
    uint32_t numStars = abs((int) format_uint32(8, headerBuffer));
    uint32_t bytesPerEntry = format_uint32(24, headerBuffer);

    // Read entries

    // Manually allocate variable sized arrays
    struct star* stars = (struct star *) malloc(numStars * sizeof(struct star));
    uint8_t *entryBuffer = (uint8_t *)malloc( bytesPerEntry);

    for (unsigned int i = 0; i < numStars; i++)
    {
        fread(entryBuffer, bytesPerEntry, 1, filePointer);
        stars[i] = entryToStar(entryBuffer);
    }

    free(entryBuffer);

    returnNumStars = numStars; // TODO: janky
    return stars;
}


// COORDINATE UTILS


float earthRotationAngle(jd)
{
    // IERS Technical Note No. 32
    // ...
}

/* reference:   "Expressions for IAU 2000 precession quantities,"
                N.Capitaine, P.T.Wallace, and J.Chapront
 */
float greenwichMeanSiderealTime()
{
    UT 1 + 24110.5493771 + 8640184.79447825 tu + 307.4771013(t − tu) + 0.092772110 t2 − 0.0000002926 t3
        − 0.00000199708 t4
        − 0.000000002454 t5
}


/* reference:   https://astrogreg.com/convert_ra_dec_to_alt_az.html
 */
void equatorialToHorizontal(float declination, float rightAscension,
                            float julianDate, float latitude, float longitude,
                            float *altitude, float *azimuth) // modifies
{
    const float GMST  = greenwichMeanSiderealTime(jd_ut);
    float hourAngle = GMST - longitude - rightAscension;

    altitude = asin(sin(latitude) * sin(declination) +
                      cos(latitude) * cos(declination) * cos(hourAngle));

    azimuth = atan2(sin(hourAngle) /
                      (cos(hourAngle) * sin(latitude) - tan(declination) * cos(latitude)));
}

// TODO: azimuth and altitude aren't the same as phi and theta in spherical coords
// phi = PI / 2 - altitude
// theta = PI / 2 - azimuth
// Flip hemisphere: add PI to phi


/* maps a point on the unit sphere, spherical coordinates:(1, theta, phi),
 * to the unit circle, polar coordinates (rCircle, thetaCircle), which lies on the plane
 * dividing the sphere across the equator. The projected point will only lie on the
 * unit sphere if 0 < phi < PI / 2 since we choose the focus point to be the south pole.
 * Reference:   https://www.atractor.pt/mat/loxodromica/saber_estereografica1-_en.html
 *              https://en.wikipedia.org/wiki/Stereographic_projection
 */
void projectStereographic(float theta, float phi,
                          float *rCircle, float *thetaCircle)
{
    const int sphereRadius = 1;

    *rCircle = tan(phi / 2);
    *thetaCircle = theta;
}


/* map a point on the unit circle to screen space coordinates
 */
void polarToScreen(float r, float theta,
                   int centerRow,int centerCol, float cellAspectRatio,
                   int *row, int *col) // modifies
{
    row = round(r * sin(theta)) + centerRow;
    col = round(r * cellAspectRatio * cos(theta)) + centerCol;
    return;
}


// RENDERING


/* attempt to get the cell aspect ratio: cell height to width
 * i.e. "how many columns form the apparent height of a row"
 */
float getCellAspectRatio()
{
    if (isatty(fileno(stdout)))
    {
        // Stdout is outputting to a terminal

        struct winsize ws;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        float cellHeight = (float) ws.ws_ypixel / ws.ws_row;
        float cellWidth  = (float) ws.ws_xpixel / ws.ws_col;

        return cellHeight / cellWidth;

    } else
    {
        // default to 2.0
        return 2.0;
    }
}

void renderMap(struct star stars[], int numStars,
               float julianDate, float latitude, float longitude,
               WINDOW *win)
{
    // get terminal dimensions
    int rows, cols;
    void getmaxyx(*win, rows, cols);
    float cellAspectRatio = getCellAspectRatio();

    // TODO: add constellation rendering

    for (int i = 0; i < numStars, ++i)
    {
        struct star* star = stars[i];

        float altitude, azimuth;
        equatorialToHorizontal(star -> declination, star -> rightAscension,
                               julianDate, latitude, longitude,
                               &altitude, &azimuth);

        // note: here we convert azimuth and altitude to
        // theta and phi in general spherical coords by
        // treating the positive y-axis as "north" for the former.
        // for the latter, phi is synonymous with the zenith angle
        float rCircle, thetaCircle;
        projectStereographic(PI/2 - azimuth, PI/2 - altitude,
                             &rCircle, &thetaCircle);

        int row, col;
        polarToScreen(rCircle, thetaCircle,
                      rows / 2, cols / 2, cellAspectRatio,
                      &row, &col);

        mvwaddch(win, row, col, '*');
    }

}


// MAIN


int main()
{

    int numStars;
    struct star *stars = readBSC5toMem("BSC5", &numStars);

    WINDOW *win = create_newwin(height, width, starty, startx);

    initscr();
    raw();
    noecho();

    // wait until enter key is pressed
    while (c = getchar() != '\n' && c != EOF)
    {
        refresh();
        renderMap(stars, numStars,
                  float julianDate, float latitude, float longitude,
                  WINDOW *win);

        sleep(0.067); // 15 fps
    }

    endwin();

    free(stars);

    return 0;
}