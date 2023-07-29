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
// #include "drawing.c"
#include "misc.h"
#include "term.h"
#include "parse_BSC5.h"

// options
static double latitude      = 0.73934145516;    // Boston, MA
static double longitude     = 5.04300525197;    // Boston, MA
static double julian_date   = 2451544.50000;    // Jan 1, 2000 00:00:00.0
static float threshold      = 3.0f;             // stars brighter than this will be rendered
static int fps              = 24;               // frames per second
static float animation_mult = 1.0f;             // real time animation speed mult (e.g. 2 is 2x real time)

// flags
static int no_unicode_flag;                     // only use ASCII characters
static int color_flag;                          // use color--not implemented yet
static int grid_flag;                           // draw an azimuthal grid

// flag for resize signal handler
static volatile bool perform_resize = false;

void update_star_positions(struct star stars[], int num_stars,
                           double julian_date, double latitude, double longitude);

void render_stars(struct star stars[], int num_stars, WINDOW *win, bool no_unicode);
void render_azimuthal_grid(WINDOW *win, bool no_unicode);

void catch_winch(int sig);
void handle_resize(WINDOW *win);

bool parse_options(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // get current time
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);
    double current_jd = datetime_to_julian_date(&lt);

    // set julian_date to current time
    julian_date = current_jd;

    // parse command line args
    bool input_error = parse_options(argc, argv);
    if (input_error) { return 1; }

    // sort stars by magnitude so "larger" stars are always rendered on top
    // reduces "flickering" when rendering many stars
    int total_stars;
    struct star *stars = parse_stars("data/BSC5", &total_stars);
    qsort(stars, total_stars, sizeof(struct star), star_magnitude_comparator);

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

        update_star_positions(stars, total_stars, julian_date, latitude, longitude);
        render_stars(stars, total_stars, win, no_unicode_flag);
        
        if (grid_flag)
        {
            render_azimuthal_grid(win, no_unicode_flag);
        }

        julian_date += (1.0 / fps) / (24 * 60 * 60) * animation_mult;

        usleep(1.0 / fps * 1000000);
    }
    
    ncurses_kill();

    free(stars);

    return 0;
}

bool parse_options(int argc, char *argv[])
{

    // https://azrael.digipen.edu/~mmead/www/mg/getopt/index.html
    int c;
    bool input_error = false;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",        required_argument,  NULL,               'a'},
            {"longitude",       required_argument,  NULL,               'o'},
            {"julian-date",     required_argument,  NULL,               'j'},
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

    return input_error;
}

void render_object_stereo(struct object_base *object, WINDOW *win,
                          bool no_unicode)
{
    double theta_sphere, phi_sphere;
    horizontal_to_spherical(object->azimuth, object->altitude,
                            &theta_sphere, &phi_sphere);

    double radius_polar, theta_polar;
    project_stereographic_north(1.0, theta_sphere, phi_sphere,
                                &radius_polar, &theta_polar);

    // if outside projection, ignore
    if (fabs(radius_polar) > 1)
    {
        return;
    }

    int row, col;
    polar_to_win(radius_polar, theta_polar,
                 win->_maxy, win->_maxx,
                 &row, &col);

    // draw object
    if (no_unicode)
    {
        mvwaddch(win, row, col, object->symbol_ASCII);
    }
    else
    {
        mvwaddstr(win, row, col, object->symbol_unicode);
    }

    // draw label
    if (object->label != NULL)
    {
        mvwaddstr(win, row - 1, col + 1, object->label);
    }
}

void update_star_positions(struct star stars[], int num_stars,
                           double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &stars[i];
        calc_star_position(star, julian_date, gmst, latitude, longitude,
                           &star->base.azimuth, &star->base.altitude);
    }

    return;
}

void render_stars(struct star stars[], int num_stars, WINDOW *win, bool no_unicode)
{
    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &stars[i];
        
        if (star->magnitude > threshold)
        {
            continue;
        }

        render_object_stereo(&star->base, win, no_unicode);
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