#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>

#include "astro.h"
#include "coord.h"
#include "drawing.h"
#include "misc.h"
#include "term.h"
#include "parse_BSC5.h"

// Options
static double latitude      = 0.73934145516;    // Boston, MA
static double longitude     = 5.04300525197;    // Boston, MA
static double julian_date   = 2451544.50000;    // Jan 1, 2000 00:00:00.0
static float threshold      = 3.0f;             // Stars brighter than this will be rendered
static float label_thresh   = 0.5f;             // Stars brighter than this will have labels
static int fps              = 24;               // Frames per second
static float animation_mult = 1.0f;             // Real time animation speed mult (e.g. 2 is 2x real time)

// Flags
static int no_unicode_flag;                     // Only use ASCII characters
static int color_flag;                          // Use color--not implemented yet
static int grid_flag;                           // Draw an azimuthal grid
static int constellation_flag;                  // Draw constellation figures

// Flag for resize signal handler
static volatile bool perform_resize = false;

void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude);

void render_azimuthal_grid(WINDOW *win, bool no_unicode);
void render_stars(WINDOW *win, struct star *star_table, int num_stars, int *num_by_mag, bool no_unicode);
void render_constellations(WINDOW *win, int **constellation_table, int num_const, struct star *star_table, bool no_unicode);

void catch_winch(int sig);
void handle_resize(WINDOW *win);

bool parse_options(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // Get current time
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);
    double current_jd = datetime_to_julian_date(&lt);

    // Set julian_date to current time
    julian_date = current_jd;

    // Parse command line args
    bool parse_error = parse_options(argc, argv);
    if (parse_error) { return 1; }

    // Data tables

    int num_stars;
    struct star *star_table = parse_stars("data/BSC5", &num_stars);

    char **name_table = parse_BSC5_names("data/BSC5_names", num_stars);
    set_star_labels(star_table, name_table, num_stars, label_thresh);

    int num_const;
    int **constellation_table = parse_BSC5_constellations("data/BSC5_constellations", &num_const);

    // Sort stars by magnitude so brighter stars are always rendered on top
    int *num_by_mag = star_numbers_by_magnitude(star_table, num_stars);

    setlocale(LC_ALL, "");          // Required for unicode rendering
    signal(SIGWINCH, catch_winch);  // Capture window resizes
    ncurses_init();

    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0);               // Non-blocking read for wgetch
    win_resize_square(win, get_cell_aspect_ratio());
    win_position_center(win);

    while (true)
    {
        // Wait until ESC is pressed
        if ((wgetch(win)) == 27)
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

        // Update object positions
        update_star_positions(star_table, num_stars, julian_date, latitude, longitude);

        // Render
        if (grid_flag)          { render_azimuthal_grid(win, no_unicode_flag); }
        if (constellation_flag) { render_constellations(win, constellation_table, num_const, star_table, no_unicode_flag); }
                                  render_stars(win, star_table, num_stars, num_by_mag, no_unicode_flag);

        // TODO: implement variable time step
        julian_date += (1.0 / fps) / (24 * 60 * 60) * animation_mult;
        usleep(1.0 / fps * 1000000);
    }
    
    ncurses_kill();

    // Free memory
    free_constellations(constellation_table, num_const);
    free_star_names(name_table, num_stars);
    free_stars(star_table, num_stars);

    return 0;
}

bool parse_options(int argc, char *argv[])
{
    // https://azrael.digipen.edu/~mmead/www/mg/getopt/index.html
    int c;
    bool parse_error = false;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",        required_argument,  NULL,               'a'},
            {"longitude",       required_argument,  NULL,               'o'},
            {"julian-date",     required_argument,  NULL,               'j'},
            {"label-thresh",    required_argument,  NULL,               'l'},
            {"threshold",       required_argument,  NULL,               't'},
            {"fps",             required_argument,  NULL,               'f'},
            {"animation-mult",  required_argument,  NULL,               'm'},
            {"no-unicode",      no_argument,        &no_unicode_flag,   1},
            {"color",           no_argument,        &color_flag,        1},
            {"grid",            no_argument,        &grid_flag,         1},
            {NULL,              0,                  NULL,               0}
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

        case 'o':
            longitude = atof(optarg);
            break;

        case 'j':
            julian_date = atof(optarg);
            break;

        case 't':
            threshold = atof(optarg);
            break;

        case 'l':
            label_thresh = atof(optarg);
            break;

        case 'f':
            fps = atoi(optarg);
            break;

        case 'm':
            animation_mult = atof(optarg);
            break;

        case '?':
            if (optopt == 0)
            {
                printf("Unrecognized long option\n");
            }
            else
            {
                printf("Unrecognized option '%c'\n", optopt);
            }
            parse_error = true;
            break;

        case ':':
            printf("Missing option for '%c'\n", optopt);
            parse_error = true;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            parse_error = true;
            break;
        }
    }

    return parse_error;
}

void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &star_table[i];
        calc_star_position(star, julian_date, gmst, latitude, longitude,
                           &star->base.azimuth, &star->base.altitude);
        // FIXME: setting the azimuth and altitude this way is probably what is
        // causing the issue in astro.h... how to fix?
    }

    return;
}

void render_object_stereo(WINDOW *win, struct object_base *object, bool no_unicode)
{
    double theta_sphere, phi_sphere;
    horizontal_to_spherical(object->azimuth, object->altitude,
                            &theta_sphere, &phi_sphere);

    double radius_polar, theta_polar;
    project_stereographic_north(1.0, theta_sphere, phi_sphere,
                                &radius_polar, &theta_polar);

    int y, x;
    polar_to_win(radius_polar, theta_polar,
                 win->_maxy, win->_maxx,
                 &y, &x);

    // Cache object coordinates
    object->y = y;
    object->x = x;

    // If outside projection, ignore
    if (fabs(radius_polar) > 1)
    {
        return;
    }

    // Draw object
    if (no_unicode)
    {
        mvwaddch(win, y, x, object->symbol_ASCII);
    }
    else
    {
        mvwaddstr(win, y, x, object->symbol_unicode);
    }

    // Draw label
    // FIXME: labels wrap around side, cause flickering
    if (object->label != NULL)
    {
        mvwaddstr(win, y - 1, x + 1, object->label);
    }
}

void render_stars(WINDOW *win, struct star *star_table, int num_stars, int *num_by_mag, bool no_unicode)
{
    for (int i = 0; i < num_stars; ++i)
    {
        int catalog_num = num_by_mag[i];
        int table_index = catalog_num - 1;

        struct star *star = &star_table[table_index];

        if (star->magnitude > threshold)
        {
            continue;
        }

        render_object_stereo(win, &star->base, no_unicode);
    }

    return;
}

void render_constellations(WINDOW *win, int **constellation_table, int num_const, struct star *star_table, bool no_unicode)
{
    for (int i = 0; i < num_const; ++i)
    {
        int *constellation = constellation_table[i];
        int num_segments = constellation[0];

        for (int j = 1; j < num_segments * 2; j += 2)
        {

            int catalog_num_A = constellation[j];
            int catalog_num_B = constellation[j + 1];

            int table_index_A = catalog_num_A - 1;
            int table_index_B = catalog_num_B - 1;

            int yA = (int) star_table[table_index_A].base.y;
            int xA = (int) star_table[table_index_A].base.x;
            int yB = (int) star_table[table_index_B].base.y;
            int xB = (int) star_table[table_index_B].base.x;

            // Draw line if reasonable length (avoid printing crazy long lines)
            // TODO: is there a cleaner way to do this (perhaps if checking if
            // one of the stars is in the window?)
            double line_length = sqrt(pow(yA - yB, 2) + pow(xA - xB, 2));
            if (line_length < 1000)
            {
               drawLineTest(win, yA, xA, yB, xB, no_unicode_flag);
            }

        }

    }
}

void render_azimuthal_grid(WINDOW *win, bool no_unicode)
{
    // TODO: fix this

    int win_height = win->_maxy;
    int win_width = win->_maxx;

    drawLineTest(win, 0, 0, win_height, win_width, no_unicode_flag);
    drawLineTest(win, win_height, 0, 0, win_width, no_unicode_flag);
    drawLineTest(win, 0, win_width / 2, win_height, win_width / 2, no_unicode_flag);
    drawLineTest(win, win_height / 2, 0, win_height / 2, win_width, no_unicode_flag);

    // DrawEllipse(win, win->_maxy/2, win->_maxx/2, 20, 20, no_unicode_flag);

    // if (no_unicode)
    // {
    //     mvwaddch(win, round(win->_maxy / 2.0), round(win->_maxx / 2.0), '+');
    // }
    // else
    // {
    //     mvwaddstr(win, round(win->_maxy / 2.0), round(win->_maxx / 2.0), "+");
    // }
}

void catch_winch(int sig)
{
    perform_resize = true;
}

void handle_resize(WINDOW *win)
{
    // Resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);
    resizeterm(y, x);

    // ???
    wclear(win);
    wrefresh(win);

    // Check cell ratio
    float aspect = get_cell_aspect_ratio();

    // Resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);

    perform_resize = false;
}