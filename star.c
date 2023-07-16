#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#include "bit_utils.c"
// #include "drawing.c"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


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
    
    uint8_t headerBuffer[28];

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

    *returnNumStars = numStars; // TODO: janky
    return stars;
}


// COORDINATE UTILS
// https://jonvoisey.net/blog/2018/07/data-converting-alt-az-to-ra-dec-derivation/
// https://astrogreg.com/convert_ra_dec_to_alt_az.html


float earthRotationAngle(float jd)
{
    float t = jd - 2451545.0;
    int d = jd - floor(jd);

    // IERS Technical Note No. 32: 5.4.4 eq. (14);
    float theta = 2 * M_PI * (d + 0.7790572732640 + 0.00273781191135448 * t);
    
    remainder(theta, 2 * M_PI);
    theta += theta < 0 ? 2 * M_PI : 0;

    return theta;
}


float greenwichMeanSiderealTime(float jd)
{
    
    // caluclate Julian centuries after J2000
    float t = ((jd - 2451545.0f)) / 36525.0f;

    // "Expressions for IAU 2000 precession quantities,"
    // N.Capitaine, P.T.Wallace, and J.Chapront
    float gmst = earthRotationAngle(jd) + 0.014506 + 4612.156534 * t +
                    1.3915817 * powf(t, 2) - 0.00000044 * powf(t, 3) -
                    0.000029956 * powf(t, 4) - 0.0000000368 * powf(t, 5);

    // normalize
    remainder(gmst, 2 * M_PI);
    gmst += gmst < 0 ? 2 * M_PI : 0;

    return gmst;
}


void equatorialToHorizontal(float declination, float rightAscension,
                            float gmst, float latitude, float longitude,
                            float *altitude, float *azimuth) // modifies
{
    float hourAngle = gmst - longitude - rightAscension;

    *altitude = (float) asin(sin(latitude) * sin(declination) +
                      cos(latitude) * cos(declination) * cos(hourAngle));

    *azimuth = (float) atan2(sin(hourAngle), cos(hourAngle) * sin(latitude) -
                                tan(declination) * cos(latitude));
}

// TODO: azimuth and altitude aren't the same as phi and theta in spherical
// coords
// phi = PI / 2 - altitude
// theta = PI / 2 - azimuth
// Flip hemisphere: add PI to phi


/* maps a point on the unit sphere, spherical coordinates:(1, theta, phi),
 * to the unit circle, polar coordinates (rCircle, thetaCircle), which lies on
 * the plane dividing the sphere across the equator. The projected point will
 * only lie on the unit sphere if 0 < phi < PI / 2 since we choose the focus
 * point to be the south pole.
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
void polarToWin(float r, float theta,
                   int centerRow,int centerCol, float cellAspectRatio,
                   int *row, int *col) // modifies
{
    *row = (int) round(r * sin(theta)) + centerRow;
    *col = (int) round(r * cellAspectRatio * cos(theta)) + centerCol;
    return;
}


// RENDERING


/* attempt to get the cell aspect ratio: cell height to width
 * i.e. "how many columns form the apparent height of a row"
 */
float getCellAspectRatio()
{
    float defaultHeight = 2.1;

    if (isatty(fileno(stdout)))
    {
        // Stdout is outputting to a terminal

        struct winsize ws;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        // in case we can't get pixel size of terminal
        if (ws.ws_ypixel == 0 || ws.ws_xpixel == 0)
        {
            return defaultHeight;
        }

        float cellHeight = (float) ws.ws_ypixel / ws.ws_row;
        float cellWidth  = (float) ws.ws_xpixel / ws.ws_col;

        return cellHeight / cellWidth;
    }

    return defaultHeight;
}

char mapMagASCII(float mag)
{
    if (mag < 1.75)
    {
        return '*';
    }
    else if (mag < 3.5)
    {
        return 'O';
    }
    else if (mag < 5.25)
    {
        return 'o';
    }
    else
    {
        return '.';
    }
}

void renderMap(struct star stars[], int numStars,
                float julianDate, float latitude, float longitude,
                WINDOW *win)
{

    // get terminal dimensions
    float cellAspectRatio = getCellAspectRatio();

    // TODO: add constellation rendering

    float gmst = greenwichMeanSiderealTime(julianDate);

    for (int i = 0; i < numStars; ++i)
    {
        struct star* star = &stars[i];

        float altitude, azimuth;
        equatorialToHorizontal(star -> declination, star -> rightAscension,
                               gmst, latitude, longitude,
                               &altitude, &azimuth);

        // note: here we convert azimuth and altitude to
        // theta and phi in general spherical coords by
        // treating the positive y-axis as "north" for the former.
        // for the latter, phi is synonymous with the zenith angle
        float rCircle, thetaCircle;
        projectStereographic(M_PI/2 - azimuth, M_PI/2 - altitude,
                             &rCircle, &thetaCircle);

        int row, col;
        polarToWin(rCircle, thetaCircle,
                   win->_maxy / 2, win->_maxx / 2, cellAspectRatio,
                   &row, &col);

        mvwaddch(win, row, col, mapMagASCII(star -> magnitude));
    }

    return;
}


// flag for signal handler
static volatile bool perform_resize = false;

void catch_winch(int sig)
{
    perform_resize = true;
}

void term_init()
{
    initscr();
    clear();
    noecho();    // input characters aren't echoed
    cbreak();    // disable line buffering
    curs_set(0); // make cursor inisible
}

void term_kill()
{
    endwin();
}

// square and center window
void center_win(WINDOW *win)
{
    float aspect = getCellAspectRatio();
    if (COLS < LINES * aspect)
    {
        wresize(win, COLS / aspect, COLS);
        mvwin(win, 0, 0);
    }
    else
    {
        wresize(win, LINES, LINES * aspect);
        mvwin(win, 0, (COLS - LINES * aspect) / 2);
    }
}

void handle_resize(WINDOW *mainwin)
{
    // reinitilize ncurses
    term_kill();
    term_init();

    erase();
    wclear(mainwin);
    refresh();

    center_win(mainwin);
    box(mainwin, 0, 0);
    
    perform_resize = false;
}

// MAIN
// https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html

int main(int argc, char *argv[])
{
    // defaults
    double latitude     = 0.73934145516; // Boston, MA
    double longitude    = 5.04300525197;
    float julianDate    = 2451544.50000; // Jan 1, 2000
    int fps             = 24;

    // flags
    // TODO: should set flags like this or use bools?
    static int f_unicode;
    static int f_color;

    int c;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",    required_argument,  NULL,       'a'},
            {"longitude",   required_argument,  NULL,       'o'},
            {"julian-date", required_argument,  NULL,       'j'},
            {"fps",         required_argument,  NULL,       'f'},
            {"unicode",     no_argument,        &f_unicode,  1},
            {"color",       no_argument,        &f_color,    1},
            {NULL,          0,                  NULL,        0}
        };

        c = getopt_long(argc, argv, ":a:l:j:f:", long_options, &option_index);
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                break;

            case 1:   
                break;

            case 'a':
                latitude = atof(optarg);
                break;

            case 'l':
                longitude = atof(optarg);
                break;

            case 'j':
                julianDate = atof(optarg);
                break;

            case 'f':
                fps = atoi(optarg);
                break;

            case '?':
                printf("Unknown option %c\n", optopt);
                break;

            case ':':
                printf("Missing option for %c\n", optopt);
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    int numStars;
    struct star *stars = readBSC5toMem("BSC5", &numStars);

    term_init();

    // Capture window resizes
    signal(SIGWINCH, catch_winch);

    // now that ncurses is initilized, calc proper size and position of win
    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0); // non-blocking read for wgetch
    center_win(win);

    // wait until ESC is pressed
    while (c = wgetch(win) != 27)
    {
        if (perform_resize)
        {
            handle_resize(win);
        }
        renderMap(stars, numStars, julianDate, latitude, longitude, win);
        wrefresh(win);
        wclear(win);
        julianDate += 0.3;
        usleep((float)1 / fps * 1000000);
    }
    
    term_kill();

    free(stars);

    return 0;
}