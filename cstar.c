#include "cstar.h"

#include "astro.h"
#include "drawing.h"
#include "coord.h"
#include "parse_BSC5.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include <math.h>

// Star magnitude mapping
static const char *mag_map_unicode_round[10]    = {"⬤", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅", "⋅"};
static const char *mag_map_unicode_diamond[10]  = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
static const char *mag_map_unicode_open[10]     = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char *mag_map_unicode_filled[10]   = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char mag_map_round_ASCII[10]       = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

static const float min_magnitude = -1.46f;
static const float max_magnitude = 7.96f;

/* Map a double `input` which lies in range [min_float, max_float]
 * to an integer which lies in range [min_int, max_int].
 */
int map_float_to_int_range(double min_float, double max_float,
                           int min_int, int max_int, double input)
{
    double percent = (input - min_float) / (max_float - min_float);
    return min_int + round((max_int - min_int) * percent);
}

struct star entry_to_star(struct entry *entry_data)
{
    struct star star_data;

    star_data.catalog_number  = (int)       entry_data->XNO;
    star_data.right_ascension =             entry_data->SRA0;
    star_data.declination     =             entry_data->SDEC0;
    star_data.ra_motion       = (double)    entry_data->XRPM;
    star_data.ra_motion       = (double)    entry_data->XDPM;
    star_data.magnitude       =             entry_data->MAG / 100.0f;

    // This is necessary to avoid printing garbage data as labels. Rendering
    // functions check if label is NULL to determine whether to print or not
    star_data.base.label = NULL;

    int symbol_index = map_float_to_int_range(min_magnitude, max_magnitude,
                                              0, 9, star_data.magnitude);

    star_data.base.symbol_ASCII     = (char)    mag_map_round_ASCII[symbol_index];
    star_data.base.symbol_unicode   = (char *)  mag_map_unicode_round[symbol_index];

    return star_data;
}

int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct star *p1 = (struct star *)v1;
    const struct star *p2 = (struct star *)v2;

    // Lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
}

struct star *generate_star_table(const char *file_path, int *num_stars_return)
{
    int num_entries;
    struct entry *entries = parse_entries(file_path, &num_entries);

    struct star *star_table = malloc(num_entries * sizeof(struct star));

    for (int i = 0; i < num_entries; ++i)
    {
        star_table[i] = entry_to_star(&entries[i]);
    }

    free(entries);

    *num_stars_return = num_entries;
    return star_table;
}

int *star_numbers_by_magnitude(struct star *star_table, int num_stars)
{
    // Create and sort a copy of the star table
    struct star *table_copy = malloc(num_stars * sizeof(struct star));
    memcpy(table_copy, star_table, num_stars * sizeof(*table_copy));
    qsort(table_copy, num_stars, sizeof(struct star), star_magnitude_comparator);

    // Create and fill array of indicies in table copy
    int *num_by_mag = malloc(num_stars * sizeof(int));
    for (int i = 0; i < num_stars; ++i)
    {
        num_by_mag[i] = table_copy[i].catalog_number;
    }

    free(table_copy);

    return num_by_mag;
}

// Star labeling

char **generate_name_table(const char *file_path, int num_stars)
{
    char **name_table = malloc(num_stars * sizeof(char *));

    FILE *file_pointer;
    file_pointer = fopen(file_path, "r");

    const unsigned BUF_SIZE = 32; // More than enough room to store any line
    char buffer[BUF_SIZE];

    while (fgets(buffer, BUF_SIZE, file_pointer))
    {
        // Split by delimiter
        int catalog_number = atoi(strtok(buffer, ","));
        char *name = strtok(NULL, ",\n");

        int table_index = catalog_number - 1;

        name_table[table_index] = malloc(BUF_SIZE * sizeof(char));
        strcpy(name_table[table_index], name);
    }

    fclose(file_pointer);

    return name_table;
}

void set_star_labels(struct star *star_table, char **name_table, int num_stars,
                     float label_thresh)
{
    for (int i = 0; i < num_stars; ++i)
    {
        // Keep NULL label if magnitude does not reach threshold
        if (star_table[i].magnitude > label_thresh)
        {
            continue;
        }

        star_table[i].base.label = name_table[i];
    }
}

// Constellations

int count_lines(FILE *file_pointer)
{
    const unsigned BUF_SIZE = 65536;

    char buffer[BUF_SIZE];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), file_pointer))
    {
        count++;
    }

    return count;
}

int **generate_constell_table(const char *file_path, int *num_const)
{
    FILE *file_pointer;
    file_pointer = fopen(file_path, "r");

    int num_constells = count_lines(file_pointer);
    rewind(file_pointer); // Reset file pointer position

    int **constell_table = malloc(num_constells * sizeof(int *));

    const unsigned BUF_SIZE = 256;
    char buffer[BUF_SIZE];

    int i = 0;
    while (fgets(buffer, BUF_SIZE, file_pointer))
    {
        // Parse constellation information

        char *name = strtok(buffer, " ");
        int num_pairs = atoi(strtok(NULL, " \n"));

        constell_table[i] = malloc((num_pairs * 2 + 1) * sizeof(int));

        // Parse constellation stars

        constell_table[i][0] = num_pairs;

        int j = 1;
        char *token;
        while (token = strtok(NULL, " \n"))
        {
            constell_table[i][j] = atoi(token);
            j++;
        }

        i++;
    }

    fclose(file_pointer);

    *num_const = num_constells;
    return constell_table;
}

// Memory freeing functions

void free_stars(struct star *arr, int size)
{
    free(arr);
    return;
}

void free_planets(struct planet *planets)
{
    free(planets);
    return;
}

void free_constells(int **arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        free(arr[i]);
    }
    free(arr);
    return;
}

void free_star_names(char **arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        free(arr[i]);
    }
    free(arr);
    return;
}

void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    for (int i = 0; i < num_stars; ++i)
    {
        struct star *star = &star_table[i];

        double right_ascension, declination;
        calc_star_position(star->right_ascension, star->ra_motion,
                           star->declination, star->dec_motion,
                           julian_date, gmst, latitude, longitude,
                           &right_ascension, &declination);

        // Convert to horizontal coordinates
        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination,
                                 gmst, latitude, longitude,
                                 &azimuth, &altitude);

        // FIXME: setting the azimuth and altitude this way is probably what is
        // causing the issue in astro.h... how to fix?
        star->base.azimuth = azimuth;
        star->base.altitude = altitude;
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

void render_stars(WINDOW *win, struct star *star_table, int num_stars, int *num_by_mag, float threshold, bool no_unicode)
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

void render_constells(WINDOW *win, int **constell_table, int num_const, struct star *star_table, bool no_unicode)
{
    for (int i = 0; i < num_const; ++i)
    {
        int *constellation = constell_table[i];
        int num_segments = constellation[0];

        for (int j = 1; j < num_segments * 2; j += 2)
        {

            int catalog_num_a = constellation[j];
            int catalog_num_b = constellation[j + 1];

            int table_index_a = catalog_num_a - 1;
            int table_index_b = catalog_num_b - 1;

            int ya = (int)star_table[table_index_a].base.y;
            int xa = (int)star_table[table_index_a].base.x;
            int yb = (int)star_table[table_index_b].base.y;
            int xb = (int)star_table[table_index_b].base.x;

            // Draw line if reasonable length (avoid printing crazy long lines)
            // TODO: is there a cleaner way to do this (perhaps if checking if
            // one of the stars is in the window?)
            double line_length = sqrt(pow(ya - yb, 2) + pow(xa - xb, 2));
            if (line_length < 1000)
            {
                draw_line_smooth(win, ya, xa, yb, xb);
            }
        }
    }
}

// Planets

static const char *planet_symbols[NUM_PLANETS] =
{
    [SUN]       = "☉",
    [MERCURY]   = "☿",
    [VENUS]     = "♀",
    [EARTH]     = "🜨",
    [MARS]      = "♂",
    [JUPITER]   = "♃",
    [SATURN]    = "♄",
    [URANUS]    = "⛢",
    [NEPTUNE]   = "♆"
};

static const char *planet_labels[NUM_PLANETS] =
{
    [SUN]       = "Sun",
    [MERCURY]   = "Mercury",
    [VENUS]     = "Venus",
    [MARS]      = "Mars",
    [JUPITER]   = "Jupiter",
    [SATURN]    = "Saturn",
    [URANUS]    = "Uranus",
    [NEPTUNE]   = "Neptune"
};

struct planet *generate_planet_table(const struct kep_elems *keplerian_elements,
                                     const struct kep_rates *keplerian_rates,
                                     const struct kep_extra *keplerian_extras)
{
    struct planet *planet_table = malloc(NUM_PLANETS * sizeof(struct planet));

    for (int i = 0; i < NUM_PLANETS; ++i)
    {
        struct planet planet_data;
        planet_data.base = (struct object_base)
        {
            .symbol_ASCII   = 'W',
            .symbol_unicode = (char *) planet_symbols[i],
            .label          = (char *) planet_labels[i],
         };
        planet_data.elements    = &keplerian_elements[i];
        planet_data.rates       = &keplerian_rates[i];
        planet_data.extras      = NULL;

        if (JUPITER <= i && i <= NEPTUNE)
        {
            planet_data.extras  = &keplerian_extras[i];
        }

        planet_table[i] = planet_data;
    }

    return planet_table;
}

void update_planet_positions(struct planet *planet_table, double julian_date,
                             double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    for (int i = 0; i < NUM_PLANETS; ++i)
    {
        // Geocentric rectangular equatorial coordinates
        double xg, yg, zg;
        calc_planet_geo_ICRF(planet_table[EARTH].elements,
                             planet_table[EARTH].rates,
                             planet_table[i].elements,
                             planet_table[i].rates, planet_table[i].extras,
                             julian_date, &xg, &yg, &zg);

        // Convert to spherical equatorial coordinates
        double right_ascension, declination;
        equatorial_rectangular_to_spherical(xg, yg, zg,
                                            &right_ascension, &declination);

        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination,
                                 gmst, latitude, longitude,
                                 &azimuth, &altitude);

        planet_table[i].base.azimuth = azimuth;
        planet_table[i].base.altitude = altitude;
    }
}

void render_planets(WINDOW *win, struct planet *planet_table, bool no_unicode)
{
    // Render planets so that closest are drawn on top
    for (int i = NUM_PLANETS - 1; i >= 0; --i)
    {
        struct planet planet_data = planet_table[i];
        render_object_stereo(win, &planet_data.base, no_unicode);
    }

    return;
}

void render_azimuthal_grid(WINDOW *win, bool no_unicode)
{
    // TODO: fix this
    int win_height = win->_maxy;
    int win_width = win->_maxx;

    int half_height = round(win_height / 2.0);
    int half_width = round(win_width / 2.0);

    int quarter_height = round(win_height / 4.0);
    int quarter_width = round(win_width / 4.0);
    //printf("%d\n", win_width);

    draw_line_smooth(win, 0, quarter_width, win_height, half_width + quarter_width);
    draw_line_smooth(win, 0, half_width + quarter_width, win_height, quarter_width);
    draw_line_smooth(win, quarter_height, 0, half_height + quarter_height, win_width);
    draw_line_smooth(win, half_height + quarter_height, 0, quarter_height, win_width);

    draw_line_smooth(win, 0, 0, win_height, win_width);
    draw_line_smooth(win, 0, win_width, win_height, 0);

    draw_line_smooth(win, 0, half_width, win_height, half_width);
    draw_line_smooth(win, half_height, 0, half_height, win_width);

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