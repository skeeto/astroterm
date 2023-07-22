#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <locale.h>
#include <time.h>

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
static const char mag_map_round_ASCII[10]       = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

static const float min_magnitude = -1.46;
static const float max_magnitude = 7.96;

struct star
{
    float catalog_number;
    float magnitude;
    double right_ascension;
    double declination;
    double altitude;
    double azimuth;
};

int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct star *p1 = (struct star *) v1;
    const struct star *p2 = (struct star *) v2;

    // remember that lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
}

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

void populate_special_chars(const char *special_chars[], int num_chars)
{
    // TODO: is there a better way to do this?

    // initialize all strings pointers to NULL
    for (int i = 0; i < num_chars; ++i)
    {
        special_chars[i] = NULL;
    }
    special_chars[424]  = "✦"; // Polaris
    special_chars[4301] = "D"; // Dubhe
    special_chars[2061] = "B"; // Betelgeuse
    return;
}

void render_stereo(struct star stars[], int num_stars,
                   const char *special_chars[],
                   bool no_unicode, float threshold,
                   WINDOW *win)
{

    for (int i = 0; i < num_stars; ++i)
    {
        struct star* star = &stars[i];

        // filter stars by magnitude
        if (star -> magnitude > threshold)
        {
            continue;
        }

        double theta_sphere, phi_sphere;
        horizontal_to_spherical(star -> azimuth, star -> altitude,
                                &theta_sphere, &phi_sphere);

        double radius_polar, theta_polar;
        project_stereographic_north(1.0, theta_sphere, phi_sphere,
                                    &radius_polar, &theta_polar);

        // if outside projection, ignore
        if (fabs(radius_polar) > 1)
        {
            continue;
        }

        int row, col;
        polar_to_win(radius_polar, theta_polar,
                   win->_maxy, win->_maxx,
                   &row, &col);

        int map_index = map_float_to_int_range(min_magnitude, max_magnitude, 0, 9, star->magnitude);

        // draw star
        if (no_unicode)
        {
            mvwaddch(win, row, col, mag_map_round_ASCII[map_index]);
        }
        else
        {
            mvwaddstr(win, row, col, mag_map_unicode_diamond[map_index]);
        }

        // special stars used for debugging
        if (special_chars[(int) star->catalog_number] != NULL)
        {
            mvwaddstr(win, row, col, special_chars[(int)star->catalog_number]);
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
    // get current time
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);
    double current_jd = datetime_to_julian_date(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                                                lt.tm_hour, lt.tm_min, lt.tm_sec);

    // defaults
    double latitude     = 0.73934145516; // Boston, MA
    double longitude    = 5.04300525197;
    double julian_date  = 2451544.50000; // Jan 1, 2000 00:00:00.0
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

    // sort stars by magnitude so "larger" stars are always rendered on top
    // reduces "flickering" when rendering many stars
    qsort(stars, total_stars, sizeof(struct star), star_magnitude_comparator);

    // initialize special character map for debugging stars
    const char  *special_chars[total_stars];
    populate_special_chars(special_chars, total_stars);

    setlocale(LC_ALL, ""); // required for unicode rendering

    signal(SIGWINCH, catch_winch); // Capture window resizes

    ncurses_init();

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

        // FIXME: rendered frames only show up starting on second frame
        werase(win);

        // FIXME: positions appear to disagree with https://stellarium-web.org/
        // by ~17:00:00.00.0

        update_star_positions(stars, total_stars, julian_date, latitude, longitude);
        render_stereo(stars, total_stars, special_chars, no_unicode, threshold, win);
        if (grid)
        {
            render_azimuthal_grid(win, no_unicode);
        }

        julian_date += 1.0 / 24;

        usleep(1.0 / fps * 1000000);
    }
    
    ncurses_kill();

    free(stars);

    return 0;
}