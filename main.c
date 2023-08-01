#include "astro.h"
#include "term.h"
#include "cstar.h"

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>

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
static int constell_flag;                  // Draw constellation figures

static volatile bool perform_resize = false;

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
    struct star *star_table = generate_star_table("data/BSC5", &num_stars);

    char **name_table = generate_name_table("data/BSC5_names", num_stars);
    set_star_labels(star_table, name_table, num_stars, label_thresh);

    int num_const;
    int **constell_table = generate_constell_table("data/BSC5_constellations", &num_const);

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
        render_stars(win, star_table, num_stars, num_by_mag, threshold, no_unicode_flag);
        if (constell_flag) { render_constells(win, constell_table, num_const, star_table, no_unicode_flag); }

        // TODO: implement variable time step
        julian_date += (1.0 / fps) / (24 * 60 * 60) * animation_mult;
        usleep(1.0 / fps * 1000000);
    }
    
    ncurses_kill();

    // Free memory
    free_constells(constell_table, num_const);
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
            {"constellations",  no_argument,        &constell_flag,       1},
            {"no-unicode",      no_argument,        &no_unicode_flag,     1},
            {"color",           no_argument,        &color_flag,          1},
            {"grid",            no_argument,        &grid_flag,           1},
            {NULL,              0,                  NULL,                 0}
        };

        // TODO: reorder optstring
        c = getopt_long(argc, argv, ":a:l:j:f:o:t:m:", long_options, &option_index);
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