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

#include "astro.h"
#include "bit.h"
#include "coord.h"
// #include "drawing.c"
#include "misc.h"
#include "term.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// flag for resize signal handler
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

struct star* read_BSC5_to_mem(const char *file_path, int *return_num_stars)
{
    /* read BSC5 into memory for efficient access
     * slightly generalized to read other catalogs in SAOTDC binary format
     * TODO: requires more generalization if we're doing that
     */

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

    *return_num_stars = num_stars;
    return stars;
}

void update_star_positions(struct star stars[], int num_stars,
                            double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);
    
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
        project_stereographic_south(1.0, M_PI/2 - star -> azimuth, M_PI/2 - star -> altitude,
                             &r_polar, &theta_polar);

        // if outside projection, ignore
        if (r_polar > 1)
        {
            continue;
        }

        int row, col;
        polar_to_win(r_polar, theta_polar,
                   win->_maxy, win->_maxx,
                   &row, &col);

        // apparent magnitudes in BSC5 range from -1.46 to 7.96
        int map_index = map_float_to_int_range(-1.46, 7.96, 0, 9, star->magnitude);
        // draw star
        if (no_unicode)
        {
            mvwaddch(win, row, col, mag_map_round_ASCII[map_index]);
        }
        else
        {
            mvwaddstr(win, row, col, mag_map_unicode_round[map_index]);
        }

        // special stars used for debugging
        if (star -> catalog_number == 424)
        {
            mvwaddstr(win, row, col, "P"); // Polaris
        }

        if (star->catalog_number == 4301)
        {
            mvwaddstr(win, row, col, "D"); // Dubhe
        }

        if (star->catalog_number == 2061)
        {
            mvwaddstr(win, row, col, "B"); // Betelgeuse
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
    
    // https://azrael.digipen.edu/~mmead/www/Courses/CS180/getopt.html
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

        // ncurses erase should occur before rendering?
        // https://stackoverflow.com/questions/68706290/how-to-reduce-flickering-lag-on-curses
        werase(win);

        update_star_positions(stars, total_stars, julian_date, latitude, longitude);
        render_stereo(stars, total_stars, no_unicode, threshold, win);
        if (grid)
        {
            render_azimuthal_grid(win, no_unicode);
        }

        julian_date += 1.0;

        usleep(1.0 / fps * 1000000);
    }
    
    term_kill();

    free(stars);

    return 0;
}