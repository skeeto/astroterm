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
#include <locale.h>
#include <termios.h>

#include "bit_utils.h"
#include "term_utils.h"
// #include "drawing.c"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// flag for signal handler
static volatile bool perform_resize = false;

// star magnitude mapping
static const char *mag_map_unicode_round[10]    = {"⬤", "⚫︎", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅"};
static const char *mag_map_unicode_diamond[10]  = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
static const char *mag_map_unicode_open[10]     = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char *mag_map_unicode_filled[10]   = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char mag_map_round_ASCII[10]       = {'O', 'o', '.', '.', '.', '.', '.', '.', '.', '.'};

struct star
{
    float catalog_number;
    float magnitude;
    double right_ascension;
    double declination;
    double altitude;
    double azimuth;
};


struct star entry_to_star(uint8_t *entry)
{
    struct star star_data;

    // BSC5 Entry format
    star_data.catalog_number    = bytes_to_float32_LE(0, entry);
    star_data.right_ascension   = bytes_to_double64_LE(4, entry);
    star_data.declination       = bytes_to_double64_LE(12, entry);
    star_data.magnitude         = (float) bytes_to_int16_LE(22, entry) / 100.0;

    return star_data;
}


/* read BSC5 into memory for efficient access
 * slightly generalized to read other catalogs in SAOTDC binary format
 * TODO: requires more generalization if we're doing that
 */
struct star* read_BSC5_to_mem(const char *file_path, int *return_num_stars)
{
    // Read header

    FILE *file_pointer;
    file_pointer = fopen(file_path, "rb");
    
    uint8_t header_buffer[28];

    fread(header_buffer, sizeof(header_buffer), 1, file_pointer);

    // We know BSC5 uses J2000 cords.
    uint32_t num_stars = abs((int) bytes_to_uint32_LE(8, header_buffer));
    uint32_t bytes_per_entry = bytes_to_uint32_LE(24, header_buffer);

    // Read entries

    // Manually allocate variable sized arrays
    struct star* stars = (struct star *) malloc(num_stars * sizeof(struct star));
    uint8_t *entry_buffer = (uint8_t *)malloc( bytes_per_entry);

    for (unsigned int i = 0; i < num_stars; i++)
    {
        fread(entry_buffer, bytes_per_entry, 1, file_pointer);
        stars[i] = entry_to_star(entry_buffer);
    }

    free(entry_buffer);

    *return_num_stars = num_stars; // TODO: janky
    return stars;
}


// COORDINATE UTILS
// https://jonvoisey.net/blog/2018/07/data-converting-alt-az-to-ra-dec-derivation/
// https://astrogreg.com/convert_ra_dec_to_alt_az.html


double earth_rotation_angle(double jd)
{
    double t = jd - 2451545.0;
    int d = jd - floor(jd);

    // IERS Technical Note No. 32: 5.4.4 eq. (14);
    double theta = 2 * M_PI * (d + 0.7790572732640 + 0.00273781191135448 * t);
    
    remainder(theta, 2 * M_PI);
    theta += theta < 0 ? 2 * M_PI : 0;

    return theta;
}


double greenwich_mean_sidereal_time(double jd)
{
    
    // caluclate Julian centuries after J2000
    double t = ((jd - 2451545.0f)) / 36525.0f;

    // "Expressions for IAU 2000 precession quantities,"
    // N.Capitaine, P.T.Wallace, and J.Chapront
    double gmst = earth_rotation_angle(jd) + 0.014506 + 4612.156534 * t +
                    1.3915817 * powf(t, 2) - 0.00000044 * powf(t, 3) -
                    0.000029956 * powf(t, 4) - 0.0000000368 * powf(t, 5);

    // normalize
    remainder(gmst, 2 * M_PI);
    gmst += gmst < 0 ? 2 * M_PI : 0;

    return gmst;
}


void equatorial_to_horizontal(double declination, double right_ascension,
                            double gmst, double latitude, double longitude,
                            double *altitude, double *azimuth) // modifies
{
    double hour_angle = gmst - longitude - right_ascension;

    *altitude = asin(sin(latitude) * sin(declination) +
                      cos(latitude) * cos(declination) * cos(hour_angle));

    *azimuth = atan2(sin(hour_angle), cos(hour_angle) * sin(latitude) -
                                tan(declination) * cos(latitude));
}

// TODO: azimuth and altitude aren't the same as phi and theta in spherical
// coords
// phi = PI / 2 - altitude
// theta = PI / 2 - azimuth
// Flip hemisphere: add PI to phi


/* maps a point on the unit sphere, spherical coordinates:(1, theta, phi),
 * to the unit circle, polar coordinates (rCircle, theta_polar), which lies on
 * the plane dividing the sphere across the equator. The projected point will
 * only lie on the unit sphere if 0 < phi < PI / 2 since we choose the focus
 * point to be the south pole.
 * Reference:   https://www.atractor.pt/mat/loxodromica/saber_estereografica1-_en.html
 *              https://en.wikipedia.org/wiki/Stereographic_projection
 */
void project_stereographic(double theta, double phi,
                          double *r_polar, double *theta_polar)
{
    const int sphereRadius = 1;

    *r_polar = tan(phi / 2);
    *theta_polar = theta;
}


/* map a point on the unit circle to screen space coordinates
 */
void polar_to_win(double r, double theta,
                int win_height, int win_width, float cell_aspect_ratio,
                int *row, int *col) // modifies
{
    *row = (int) round(r * win_height / 2 * sin(theta)) + win_height / 2;
    *col = (int) round(r * win_width / 2 * cos(theta)) + win_width / 2;
    return;
}


// RENDERING


/* attempt to get the cell aspect ratio: cell height to width
 * i.e. "how many columns form the apparent height of a row"
 */
float get_cell_aspect_ratio()
{
    float default_height = 2.15;

    if (isatty(fileno(stdout)))
    {
        // Stdout is outputting to a terminal

        struct winsize ws;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        // in case we can't get pixel size of terminal
        if (ws.ws_ypixel == 0 || ws.ws_xpixel == 0)
        {
            return default_height;
        }

        float cell_height = (float) ws.ws_ypixel / ws.ws_row;
        float cell_width  = (float) ws.ws_xpixel / ws.ws_col;

        return cell_height / cell_width;
    }

    return default_height;
}

char map_mag_ASCII(const char map[10], float mag)
{
    // apparent magnitudes in BSC5 range from -1.46 to 7.96
    // this way we map all possible rounded values between 0 and 9 inclusive
    int index = floor(mag) + 2;
    return map[index];
}

char *map_mag_unicode(const char *map[10], float mag)
{
    // apparent magnitudes in BSC5 range from -1.46 to 7.96
    // this way we map all possible rounded values between 0 and 9 inclusive
    int index = floor(mag) + 2;
    return (char *) map[index];
}

void update_star_positions(struct star stars[], int num_stars,
                            double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time(julian_date);
    
    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &stars[i];

        double altitude, azimuth;
        equatorial_to_horizontal(star -> declination, star -> right_ascension,
                               gmst, latitude, longitude,
                               &altitude, &azimuth);

        star -> altitude = altitude;
        star -> azimuth = azimuth;
    }

    return;
}

void render_stereo(struct star stars[], int num_stars,
                   bool no_unicode, float threshold,
                   WINDOW *win)
{

    // get terminal dimensions
    float cell_aspect_ratio = get_cell_aspect_ratio();

    // TODO: add constellation rendering
    float lowest_mag = -10;
    for (int i = 0; i < num_stars; ++i)
    {
        struct star* star = &stars[i];

        // filter stars by magnitude
        if (star -> magnitude > threshold)
        {
            continue;
        }

        // note: here we convert azimuth and altitude to
        // theta and phi in general spherical coords by
        // treating the positive y-axis as "north" for the former.
        // for the latter, phi is synonymous with the zenith angle
        double r_polar, theta_polar;
        project_stereographic(M_PI/2 - star -> azimuth, M_PI/2 - star -> altitude,
                             &r_polar, &theta_polar);

        // if outside projection, ignore
        if (r_polar > 1)
        {
            continue;
        }

        int row, col;
        polar_to_win(r_polar, theta_polar,
                   win->_maxy, win->_maxx, cell_aspect_ratio,
                   &row, &col);

        // draw star
        if (no_unicode)
        {
            mvwaddch(win, row, col, map_mag_ASCII(mag_map_round_ASCII, star->magnitude));
        }
        else
        {
            mvwaddstr(win, row, col, map_mag_unicode(mag_map_unicode_round, star->magnitude));
        }
    }

    return;
}

void render_azimuthal_grid(WINDOW *win, bool no_unicode)
{
    if (no_unicode)
    {
        mvwaddch(win, win->_maxy / 2, win->_maxx / 2, '+');
    }
    else
    {
        mvwaddstr(win, win->_maxy / 2, win->_maxx / 2, "＋");
    }
}



void catch_winch(int sig)
{
    perform_resize = true;
}

void handle_resize(WINDOW *win)
{
    // resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);
    resizeterm(y, x);

    // ???
    wclear(win);
    wrefresh(win);

    // check cell ratio
    float aspect = get_cell_aspect_ratio();

    // resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);

    perform_resize = false;
}

// MAIN
// https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html

int main(int argc, char *argv[])
{
    // defaults
    double latitude     = 0.73934145516; // Boston, MA
    double longitude    = 5.04300525197;
    double julian_date  = 2451544.50000; // Jan 1, 2000
    float threshold     = 3.0f;
    int fps             = 24;

    static int no_unicode;
    static int color;
    static int grid;

    int c;
    bool input_error = false;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",    required_argument,  NULL,       'a'},
            {"longitude",   required_argument,  NULL,       'o'},
            {"julian-date", required_argument,  NULL,       'j'},
            {"threshold",   required_argument,  NULL,       't'},
            {"fps",         required_argument,  NULL,       'f'},
            {"no-unicode",  no_argument,        &no_unicode,  1},
            {"color",       no_argument,        &color,       1},
            {"grid",        no_argument,        &grid,        1},
            {NULL,          0,                  NULL,         0}
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
                julian_date = atof(optarg);
                break;

            case 't':
                threshold = atof(optarg);
                break;

            case 'f':
                fps = atoi(optarg);
                break;

            case '?':
                printf("Unrecognized option '%c'\n", optopt);
                input_error = true;
                break;

            case ':':
                printf("Missing option for '%c'\n", optopt);
                input_error = true;
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
                input_error = true;
                break;
        }
    }

    if (input_error)
    {
        return 1;
    }

    int total_stars;
    struct star *stars = read_BSC5_to_mem("data/BSC5", &total_stars);

    setlocale(LC_ALL, ""); // required for unicode rendering

    signal(SIGWINCH, catch_winch); // Capture window resizes

    term_init();

    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0); // non-blocking read for wgetch
    win_resize_square(win, get_cell_aspect_ratio());
    win_position_center(win);

    while (true)
    {
        
        // wait until ESC is pressed
        if ((c = wgetch(win)) == 27)
        {
            break;
        }

        if (perform_resize)
        {
            handle_resize(win);
        }

        // ncruses erase should occur before rendering?
        // https://stackoverflow.com/questions/68706290/how-to-reduce-flickering-lag-on-curses
        werase(win);

        update_star_positions(stars, total_stars, julian_date, latitude, longitude);
        render_stereo(stars, total_stars, no_unicode, threshold, win);
        if (grid)
        {
            render_azimuthal_grid(win, no_unicode);
        }

        julian_date += 0.1;

        usleep(1.0 / fps * 1000000);
    }
    
    term_kill();

    free(stars);

    return 0;
}