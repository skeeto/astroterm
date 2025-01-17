#include "city.h"
#include "core.h"
#include "core_position.h"
#include "core_render.h"
#include "data/keplerian_elements.h"
#include "macros.h"
#include "parse_BSC5.h"
#include "stopwatch.h"
#include "term.h"
#include "version.h"

// Embedded data generated during build
#include "bsc5.h"
#include "bsc5_constellations.h"
#include "bsc5_names.h"

#include <curses.h>
#include "optparse.c"

#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static void catch_winch(int sig);
static void resize_ncurses(void);
static void resize_meta(WINDOW *win);
static void resize_main(WINDOW *win, const struct Conf *config);
static void parse_options(int argc, char *argv[], struct Conf *config);
static void convert_options(struct Conf *config);
static const char *get_timezone(const struct tm *local_time);
static void render_metadata(WINDOW *win, const struct Conf *config);

// Track if we need to resize the curses window
static volatile bool perform_resize = false;
#ifdef _WIN32
// Track console size on windows
static COORD winsize;
#endif

// Track current simulation time (UTC)
// Default to current time in dt_string_utc is NULL
static double julian_date = 0.0;
static double julian_date_start = 0.0; // Note of when we started

int main(int argc, char *argv[])
{
    // Default config
    struct Conf config = {
        .longitude = 0.0,
        .latitude = 0.0,
        .dt_string_utc = NULL,
        .threshold = 5.0f,
        .label_thresh = 0.25f,
        .fps = 24,
        .speed = 1.0f,
        .aspect_ratio = 0.0,
        .quit_on_any = false,
        .unicode = false,
        .color = false,
        .grid = false,
        .constell = false,
        .metadata = false,
    };

    // Parse command line args and convert to internal representations
    parse_options(argc, argv, &config);
    convert_options(&config);

    // Time for each frame in microseconds
    unsigned long dt = (unsigned long)(1.0 / config.fps * 1.0E6);

    // Initialize data structs
    unsigned int num_stars, num_const;

    struct Entry *BSC5_entries = NULL;
    struct StarName *name_table = NULL;
    struct Constell *constell_table = NULL;
    struct Star *star_table = NULL;
    struct Planet *planet_table = NULL;
    struct Moon moon_object;
    int *num_by_mag = NULL;

    // Track success of functions
    bool s = true;

    // Generated BSC5 data during build in bsc5_xxx.h:
    //
    // uint8_t bsc5_xxx[];
    // size_t bsc5_xxx_len;

    s = s && parse_entries(bsc5, bsc5_len, &BSC5_entries, &num_stars);
    s = s && generate_name_table(bsc5_names, bsc5_names_len, &name_table, num_stars);
    s = s && generate_constell_table(bsc5_constellations, bsc5_constellations_len, &constell_table, &num_const);
    s = s && generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    s = s && generate_planet_table(&planet_table, planet_elements, planet_rates, planet_extras);
    s = s && generate_moon_object(&moon_object, &moon_elements, &moon_rates);
    s = s && star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);

    if (!s)
    {
        // At least one of the above functions failed, exit
        exit(EXIT_FAILURE);
    }

    // This memory is no longer needed
    free(BSC5_entries);

    // Terminal/System settings
    setlocale(LC_ALL, ""); // Required for unicode rendering
#ifndef _WIN32
    signal(SIGWINCH, catch_winch); // Capture window resizes
#endif
    tzset(); // Initialize timezone information

    // Ncurses initialization
    ncurses_init(config.color);

    // Main (projection) window
    WINDOW *main_win = newwin(0, 0, 0, 0);
    resize_main(main_win, &config);

    // Metadata window
    WINDOW *metadata_win = newwin(0, 0, 0, 0); // Position at top left
    if (config.metadata)
    {
        resize_meta(metadata_win);
    }

    // Render loop
    while (true)
    {
        struct SwTimestamp frame_begin;
        sw_gettime(&frame_begin);

#ifdef _WIN32
        // Use this function to catch console resizes on Windows
        perform_resize = check_console_window_resize_event(&winsize);
#endif

        if (perform_resize)
        {
            resize_ncurses();
            resize_main(main_win, &config);
            if (config.metadata)
            {
                resize_meta(metadata_win);
            }
            doupdate();

            perform_resize = false;
        }
        else
        {
            werase(metadata_win);
            werase(main_win);
        }

        // Update object positions
        update_star_positions(star_table, num_stars, julian_date, config.latitude, config.longitude);
        update_planet_positions(planet_table, julian_date, config.latitude, config.longitude);
        update_moon_position(&moon_object, julian_date, config.latitude, config.longitude);
        update_moon_phase(&moon_object, julian_date, config.latitude);

        // Render objects
        render_stars_stereo(main_win, &config, star_table, num_stars, num_by_mag);
        if (config.constell)
        {
            render_constells(main_win, &config, &constell_table, num_const, star_table);
        }
        render_planets_stereo(main_win, &config, planet_table);
        render_moon_stereo(main_win, &config, moon_object);
        if (config.grid)
        {
            render_azimuthal_grid(main_win, &config);
        }
        else
        {
            render_cardinal_directions(main_win, &config);
        }

        // Render metadata
        if (config.metadata)
        {
            render_metadata(metadata_win, &config);
        }

        // Exit if ESC or q is pressed
        int ch = getch();
        if (ch != ERR && (ch == 27 || ch == 'q' || config.quit_on_any))
        {
            break;
        }

        // Use double buffering to avoid flickering while updating
        wnoutrefresh(main_win);
        if (config.metadata)
        {
            wnoutrefresh(metadata_win);
        }
        doupdate();

        // TODO: this timing scheme *should* minimize any drift or divergence
        // between simulation time and realtime. Check this to make sure.

        // Increment "simulation" time
        const double microsec_per_day = 24.0 * 60.0 * 60.0 * 1.0E6;
        julian_date += (double)dt / microsec_per_day * config.speed;

        // Determine time it took to update positions and render to screen
        struct SwTimestamp frame_end;
        sw_gettime(&frame_end);

        unsigned long long frame_time;
        sw_timediff_usec(frame_end, frame_begin, &frame_time);

        // If updating the frame took less time than the time between frames,
        // wait the rest of the time
        if (frame_time < dt)
        {
            sw_sleep(dt - frame_time);
        }
    }

    // Clean up

    ncurses_kill();

    free_constells(constell_table, num_const);
    free_stars(star_table, num_stars);
    free_planets(planet_table, NUM_PLANETS);
    free_moon_object(moon_object);
    free_star_names(name_table, num_stars);

    return EXIT_SUCCESS;
}

static void usage(void)
{
    char usage[] =
"View stars, planets, and more, right in your terminal! ✨🪐\n"
"\n"
"Usage: astroterm [OPTION]...\n"
"\n"
"  -a, --latitude FLOAT     Observer latitude [-90°, 90°] (0.0)\n"
"  -o, --longitude FLOAT     Observer longitude [-180°, 180°] (0.0)\n"
"  -d, --datetime yyyy-mm-ddThh:mm:ss\n"
"                            Observation datetime in UTC\n"
"  -t, --threshold FLOAT     Minimum magnitude to render a star (0.5)\n"
"  -l, --label-thresh FLOAT\n"
"                            Minimum magnitude to label a star (0.25)\n"
"  -f, --fps N               Frames per second (24)\n"
"  -s, --speed FLOAT         Animation speed multiplier (1.0)\n"
"  -c, --color               Enable terminal colors\n"
"  -C, --constellations      Draw constellation stick figures\n"
"  -g, --grid                Draw an azimuthal grid\n"
"  -u, --unicode             Use unicode characters\n"
"  -q, --quit-on-any         Quit on any key (default quit on 'q' or ESC)\n"
"  -m, --metadata            Display metadata\n"
"  -r, --aspect-ratio FLOAT  Override calculated terminal cell aspect ratio\n"
"  -h, --help                Print this help message\n"
"  -i, --city NAME           Use the coordinate the provided city\n"
"  -v, --version             Display version info and exit\n";
    fwrite(usage, sizeof(usage)-1, 1, stdout);
}

void parse_options(int argc, char *argv[], struct Conf *config)
{
    struct optparse_long longopts[] = {
        {"latitude",       'a', OPTPARSE_REQUIRED},
        {"longitude",      'o', OPTPARSE_REQUIRED},
        {"datetime",       'd', OPTPARSE_REQUIRED},
        {"threshold",      't', OPTPARSE_REQUIRED},
        {"label-thresh",   'l', OPTPARSE_REQUIRED},
        {"fps",            'f', OPTPARSE_REQUIRED},
        {"speed",          's', OPTPARSE_REQUIRED},
        {"color",          'c', OPTPARSE_NONE},
        {"constellations", 'C', OPTPARSE_NONE},
        {"grid",           'g', OPTPARSE_NONE},
        {"unicode",        'u', OPTPARSE_NONE},
        {"quit-on-any",    'q', OPTPARSE_NONE},
        {"metadata",       'm', OPTPARSE_NONE},
        {"aspect-ratio",   'r', OPTPARSE_REQUIRED},
        {"city",           'i', OPTPARSE_REQUIRED},
        {"help",           'h', OPTPARSE_NONE},
        {"version",        'v', OPTPARSE_NONE},
        {0}
    };

    struct optparse options;
    optparse_init(&options, argv);
    for (int opt; (opt = optparse_long(&options, longopts, NULL)) != -1;)
    {
        switch (opt)
        {
        case 'a':
            config->latitude = strtod(options.optarg, NULL);
            if (config->latitude < -90 || config->latitude > 90)
            {
                fputs("ERROR: Latitude out of range [-90°, 90°]\n", stderr);
                exit(EXIT_FAILURE);
            }
            break;
        case 'o':
            config->longitude = strtod(options.optarg, NULL);
            if (config->longitude < -180 || config->longitude > 180)
            {
                fputs("ERROR: Longitude out of range [-180°, 180°]\n", stderr);
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            config->threshold = strtod(options.optarg, NULL);
            break;
        case 't':
            config->dt_string_utc = options.optarg;
            break;
        case 'l':
            config->label_thresh = strtod(options.optarg, NULL);
            break;
        case 'f':
            config->fps = atoi(options.optarg);
            if (config->fps < 1)
            {
                fputs("ERROR: FPS must be greater than or equal to 1\n",
                      stderr);
                exit(EXIT_FAILURE);
            }
            break;
        case 's':
            config->speed = strtod(options.optarg, NULL);
            break;
        case 'c':
            config->color = true;
            break;
        case 'C':
            config->constell = true;
            break;
        case 'g':
            config->grid = true;
            break;
        case 'u':
            config->unicode = true;
            break;
        case 'q':
            config->quit_on_any = true;
            break;
        case 'm':
            config->metadata = true;
            break;
        case 'r':
            config->aspect_ratio = strtod(options.optarg, NULL);
            break;
        case 'i':
            CityData *city = get_city(options.optarg);
            if (!city)
            {
                fprintf(stderr, "ERROR: Could not find city \"%s\"\n",
                        options.optarg);
                exit(EXIT_FAILURE);
            }
            config->latitude = city->latitude;
            config->longitude = city->longitude;
            free_city(city);
            break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        case 'v':
            printf("%s %s\n", PROJ_NAME, PROJ_VERSION);
            exit(EXIT_SUCCESS);
        case '?':
            fprintf(stderr, "ERROR: %s\n", options.errmsg);
            printf("Try '--help' for more information.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void convert_options(struct Conf *config)
{
    // Convert longitude and latitude to radians
    config->longitude *= M_PI / 180.0;
    config->latitude *= M_PI / 180.0;

    // Convert Gregorian calendar date to Julian date
    if (config->dt_string_utc == NULL)
    {
        // Set julian date to current time
        julian_date_start = current_julian_date();
        julian_date = julian_date_start;
    }
    else
    {
        struct tm datetime;
        bool parse_success = string_to_time(config->dt_string_utc, &datetime);
        if (!parse_success)
        {
            printf("ERROR: Unable to parse datetime string '%s'\nDatetimes "
                   "must be in form <yyyy-mm-ddThh:mm:ss>\n",
                   config->dt_string_utc);
            exit(EXIT_FAILURE);
        }
        julian_date_start = datetime_to_julian_date(&datetime);
        julian_date = julian_date_start;
    }

    return;
}

void catch_winch(int sig)
{
    (void)sig;
    perform_resize = true;
}

void resize_ncurses(void)
{
    // Resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);

#ifdef _WIN32
    resize_term(winsize.Y, winsize.X);
#else
    resize_term(y, x);
#endif
}

void resize_main(WINDOW *win, const struct Conf *config)
{
    // Clear the window before resizing
    werase(win);
#ifndef _WIN32
    wnoutrefresh(win);
#endif

    // Check cell ratio
    float aspect;
    if (config->aspect_ratio)
    {
        aspect = config->aspect_ratio;
    }
    else
    {
        aspect = get_cell_aspect_ratio();
    }

    // Resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);
#ifdef _WIN32
    wnoutrefresh(win);
#endif
}

void resize_meta(WINDOW *win)
{
    // Clear the window before resizing
    werase(win);
#ifndef _WIN32
    wnoutrefresh(win);
#endif

    const int meta_lines = 6; // Allows for 6 rows
    const int meta_cols = 45; // Set to allow enough room for longest line (elapsed time)

    wresize(win, MIN(LINES, meta_lines), MIN(COLS, meta_cols));
#ifdef _WIN32
    wnoutrefresh(win);
#endif
}

const char *get_timezone(const struct tm *local_time)
{
#ifdef _WIN32
    // Windows-specific code
    TIME_ZONE_INFORMATION tz_info;
    GetTimeZoneInformation(&tz_info);

    static char tzbuf[8];
    char sign = tz_info.Bias > 0 ? '-' : '+';
    long hours = labs(tz_info.Bias) / 60;
    long minutes = labs(tz_info.Bias) % 60;
    snprintf(tzbuf, sizeof(tzbuf), "%c%02ld:%02ld", sign, hours, minutes);
    return tzbuf;
#else
    // Unix-like systems (Linux/macOS) code
    extern char *tzname[2]; // tzname[0] is standard, tzname[1] is DST
    return local_time->tm_isdst > 0 ? tzname[1] : tzname[0];
#endif
}

void render_metadata(WINDOW *win, const struct Conf *config)
{
    // Gregorian Date (local time)

    // Convert sim julian date (UTC) to local time
    const double JULIAN_DATE_EPOCH = 2440587.5;
    time_t utc_time = (time_t)((julian_date - JULIAN_DATE_EPOCH) * 86400);
    const struct tm *local_time = localtime(&utc_time);
    if (local_time == NULL)
    {
        // Default to UTC if conversion fails
        local_time = gmtime(&utc_time);
    }

    int year = local_time->tm_year + 1900; // tm_year is years since 1900
    int month = local_time->tm_mon + 1;    // tm_mon is months since January (0-11)
    int day = local_time->tm_mday;         // Day of the month
    int hour = local_time->tm_hour;        // Hour (0-23)
    int minute = local_time->tm_min;       // Minute (0-59)

    const char *timezone = get_timezone(local_time);
    mvwprintw(win, 0, 0, "Date (%s): \t%02d-%02d-%04d %02d:%02d", timezone, day, month, year, hour, minute);

    // Zodiac
    const char *zodiac_name = get_zodiac_sign(month, day);
    const char *zodiac_symbol = get_zodiac_symbol(month, day);
    if (config->unicode)
    {
        mvwprintw(win, 1, 0, "Zodiac: \t%s %s", zodiac_name, zodiac_symbol);
    }
    else
    {
        mvwprintw(win, 1, 0, "Zodiac: \t%s", zodiac_name);
    }

    // Lunar phase
    double age = calc_moon_age(julian_date);
    enum MoonPhase phase = moon_age_to_phase(age);
    const char *lunar_phase = get_moon_phase_name(phase);
    mvwprintw(win, 2, 0, "Lunar Phase: \t%s", lunar_phase);

    // Lat and Lon (convert back to degrees)
    int deg, min;
    double sec;
    decimal_to_dms(config->latitude * 180 / M_PI, &deg, &min, &sec);
    mvwprintw(win, 3, 0, "Latitude: \t%d° %d' %.2f\"", deg, min, sec);

    // Longitude
    decimal_to_dms(config->longitude * 180 / M_PI, &deg, &min, &sec);
    mvwprintw(win, 4, 0, "Longitude: \t%d° %d' %.2f\"", deg, min, sec);

    // Elapsed time
    int eyears, edays, ehours, emins, esecs;
    elapsed_time_to_components(julian_date - julian_date_start, &eyears, &edays, &ehours, &emins, &esecs);
    const char *year_label = (eyears == 1) ? " year" : "years";
    const char *day_label = (edays == 1) ? " day" : "days";

    // Display elapsed time with proper labels
    mvwprintw(win, 5, 0, "Elapsed Time: \t%03d %s, %03d %s, %02d:%02d:%02d", eyears, year_label, edays, day_label, ehours,
              emins, esecs);

    return;
}
