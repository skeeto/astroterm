#include "core.h"
#include "core_position.h"
#include "core_render.h"

#include "data/keplerian_elements.h"
#include "parse_BSC5.h"
#include "stopwatch.h"
#include "term.h"

// Embedded data generated during build
#include "bsc5_constellations.h"
#include "bsc5_data.h"
#include "bsc5_names.h"

// Third part libraries
#include "argtable3.h"

#include <getopt.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

static volatile bool perform_resize = false;

static void catch_winch(int sig);
static void handle_resize(WINDOW *win);
static void parse_options(int argc, char *argv[], struct conf *config);
static void convert_options(struct conf *config);

int main(int argc, char *argv[])
{
    struct conf config = {
        .longitude = -71.057083, // Boston, MA
        .latitude = 42.361145,
        .dt_string_utc = NULL,
        .threshold = 3.0f,
        .label_thresh = 0.5f,
        .fps = 24,
        .animation_mult = 1.0f,
        .julian_date = 0.0,
        .unicode_flag = true,
        .color_flag = false,
        .grid_flag = false,
        .constell_flag = false,
    };

    // Parse command line args and convert to internal representations
    parse_options(argc, argv, &config);
    convert_options(&config);

    // Time for each frame in microseconds
    unsigned long dt = (unsigned long)(1.0 / config.fps * 1.0E6);

    // Initialize data structs
    unsigned int num_stars, num_const;

    struct entry *BSC5_entries;
    struct star_name *name_table;
    struct constell *constell_table;
    struct star *star_table;
    struct planet *planet_table;
    struct moon moon_object;
    int *num_by_mag;

    // Track success of functions
    bool s = true;

    // Generated BSC5 data during build in bsc5_xxx.h:
    //
    // uint8_t bsc5_xxx[];
    // size_t bsc5_xxx_len;

    s = s && parse_entries(bsc5_data, bsc5_data_len, &BSC5_entries, &num_stars);
    s = s && generate_name_table(bsc5_names, bsc5_names_len, &name_table, num_stars);
    s = s && generate_constell_table(bsc5_constellations, bsc5_constellations_len, &constell_table, &num_const);
    s = s && generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    s = s && generate_planet_table(&planet_table, planet_elements, planet_rates, planet_extras);
    s = s && generate_moon_object(&moon_object, &moon_elements, &moon_rates);
    s = s && star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);

    if (!s)
    {
        // At least one of the above functions failed, abort
        abort();
    }

    // This memory is no longer needed
    free(BSC5_entries);
    free_star_names(name_table, num_stars);

    // Terminal/System settings
    setlocale(LC_ALL, "");         // Required for unicode rendering
    signal(SIGWINCH, catch_winch); // Capture window resizes

    // Ncurses initialization
    ncurses_init(config.color_flag != 0);
    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0); // Non-blocking read for wgetch
    win_resize_square(win, get_cell_aspect_ratio());
    win_position_center(win);

    // Render loop
    while (true)
    {
        struct sw_timestamp frame_begin;
        sw_gettime(&frame_begin);

        werase(win);

        if (perform_resize)
        {
            // Putting this after erasing the window reduces flickering
            handle_resize(win);
        }

        // Update object positions
        update_star_positions(star_table, num_stars, config.julian_date, config.latitude, config.longitude);
        update_planet_positions(planet_table, config.julian_date, config.latitude, config.longitude);
        update_moon_position(&moon_object, config.julian_date, config.latitude, config.longitude);
        update_moon_phase(&moon_object, config.julian_date, config.latitude);

        // Render
        render_stars_stereo(win, &config, star_table, num_stars, num_by_mag);
        if (config.constell_flag != 0)
        {
            render_constells(win, &config, &constell_table, num_const, star_table);
        }
        render_planets_stereo(win, &config, planet_table);
        render_moon_stereo(win, &config, moon_object);
        if (config.grid_flag != 0)
        {
            render_azimuthal_grid(win, &config);
        }
        else
        {
            render_cardinal_directions(win, &config);
        }

        // Exit if ESC or q is pressed
        int ch = wgetch(win);
        if (ch == 27 || ch == 'q')
        {
            // Note: wgetch also calls wrefresh(win), so we want this at the
            // bottom after the virtual screen is updated
            break;
        }

        // TODO: this timing scheme *should* minimize any drift or divergence
        // between simulation time and realtime. Check this to make sure.

        // Increment "simulation" time
        const double microsec_per_day = 24.0 * 60.0 * 60.0 * 1.0E6;
        config.julian_date += (double)dt / microsec_per_day * config.animation_mult;

        // Determine time it took to update positions and render to screen
        struct sw_timestamp frame_end;
        sw_gettime(&frame_end);

        unsigned long long frame_time;
        sw_timediff_usec(frame_end, frame_begin, &frame_time);

        if (frame_time < dt)
        {
            sw_sleep(dt - frame_time);
        }
    }

    ncurses_kill();

    free_constells(constell_table, num_const);
    free_stars(star_table, num_stars);
    free_planets(planet_table, NUM_PLANETS);
    free_moon_object(moon_object);

    return 0;
}

void parse_options(int argc, char *argv[], struct conf *config)
{
    // Define Argtable3 option structures
    struct arg_dbl *latitude_arg = arg_dbl0("a", "latitude", "<degrees>", "Observer latitude [-90°, 90°] (default: 42.361145)");
    struct arg_dbl *longitude_arg =
        arg_dbl0("o", "longitude", "<degrees>", "Observer longitude [-180°, 180°] (default: -71.057083)");
    struct arg_str *datetime_arg = arg_str0("d", "datetime", "<yyyy-mm-ddThh:mm:ss>", "Observation time in UTC");
    struct arg_dbl *threshold_arg =
        arg_dbl0("t", "threshold", "<float>", "Stars brighter than this will be drawn (default: 3.0)");
    struct arg_dbl *label_arg =
        arg_dbl0("l", "label-thresh", "<float>", "Stars brighter than this will have labels (default: 0.5)");
    struct arg_int *fps_arg = arg_int0("f", "fps", "<int>", "Frames per second (default: 24)");
    struct arg_dbl *anim_arg =
        arg_dbl0("m", "animation-mult", "<float>", "Real time animation speed multiplier (default: 1.0)");
    struct arg_lit *color_arg = arg_lit0(NULL, "color", "Enable terminal colors");
    struct arg_lit *constell_arg = arg_lit0(NULL, "constellations",
                                            "Draw constellations stick figures. Note: a constellation is only "
                                            "drawn if all stars in the figure are visible (over the threshold).");
    struct arg_lit *grid_arg = arg_lit0(NULL, "grid", "Draw an azimuthal grid");
    struct arg_lit *ascii_arg = arg_lit0(NULL, "no-unicode", "Only use ASCII characters");
    struct arg_lit *help_arg = arg_lit0("h", "help", "Print this help message");
    struct arg_end *end = arg_end(20);

    // Create argtable array
    void *argtable[] = {latitude_arg, longitude_arg, datetime_arg, threshold_arg, label_arg, fps_arg, anim_arg,
                        color_arg,    constell_arg,  grid_arg,     ascii_arg,     help_arg,  end};

    // Parse the arguments
    int nerrors = arg_parse(argc, argv, argtable);

    if (help_arg->count > 0)
    {
        printf("View stars, planets, and more, right in your terminal! ✨🪐\n\n");
        printf("Usage: starsaver [OPTION]...\n\n");
        arg_print_glossary_gnu(stdout, argtable);
        exit(EXIT_SUCCESS);
    }

    if (nerrors > 0)
    {
        arg_print_errors(stderr, end, argv[0]);
        printf("Try '--help' for more information.\n");
        exit(EXIT_FAILURE);
    }

    // Assign parsed values to global variables
    if (latitude_arg->count > 0)
    {
        config->latitude = latitude_arg->dval[0];
        if (config->latitude < -90 || config->latitude > 90)
        {
            fprintf(stderr, "ERROR: Latitude out of range [-90°, 90°]\n");
            exit(EXIT_FAILURE);
        }
    }

    if (longitude_arg->count > 0)
    {
        config->longitude = longitude_arg->dval[0];
        if (config->longitude < -180 || config->longitude > 180)
        {
            fprintf(stderr, "ERROR: Longitude out of range [-180°, 180°]\n");
            exit(EXIT_FAILURE);
        }
    }

    if (datetime_arg->count > 0)
    {
        config->dt_string_utc = datetime_arg->sval[0];
    }

    if (threshold_arg->count > 0)
    {
        config->threshold = (float)threshold_arg->dval[0];
    }

    if (label_arg->count > 0)
    {
        config->label_thresh = (float)label_arg->dval[0];
    }

    if (fps_arg->count > 0)
    {
        config->fps = fps_arg->ival[0];
        if (config->fps < 1)
        {
            fprintf(stderr, "ERROR: FPS must be greater than or equal to 1\n");
            exit(EXIT_FAILURE);
        }
    }

    if (anim_arg->count > 0)
    {
        config->animation_mult = (float)anim_arg->dval[0];
    }

    if (color_arg->count > 0)
    {
        config->color_flag = TRUE;
    }

    if (constell_arg->count > 0)
    {
        config->constell_flag = TRUE;
    }

    if (grid_arg->count > 0)
    {
        config->grid_flag = TRUE;
    }

    if (ascii_arg->count > 0)
    {
        config->unicode_flag = FALSE;
    }

    // Free Argtable resources
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
}

void convert_options(struct conf *config)
{
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

    // Convert longitude and latitude to radians
    config->longitude *= M_PI / 180.0;
    config->latitude *= M_PI / 180.0;

    // Convert Gregorian calendar date to Julian date
    if (config->dt_string_utc == NULL)
    {
        // Set julian date to current time
        config->julian_date = current_julian_date();
    }
    else
    {
        struct tm datetime;
        bool parse_success = string_to_time(config->dt_string_utc, &datetime);
        if (!parse_success)
        {
            printf("ERROR: Unable to parse datetime string '%s'\nDatetimes "
                   "must be in form <yyyy-mm-ddThh:mm:ss>",
                   config->dt_string_utc);
            exit(EXIT_FAILURE);
        }
        config->julian_date = datetime_to_julian_date(&datetime);
    }

    return;
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
