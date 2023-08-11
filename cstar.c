#include "cstar.h"

#include "astro.h"
#include "term.h"
#include "drawing.h"
#include "coord.h"
#include "parse_BSC5.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ncurses.h>
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

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

    // This is necessary to avoid adding color to the star. Rendering
    // functions check if color_pair is 0 to determine whether to add color
    star_data.base.color_pair = 0;

    // Star magnitude mapping
    static const char *mag_map_unicode_round[10]    = {"⬤", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅", "⋅"};
    static const char *mag_map_unicode_diamond[10]  = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
    static const char *mag_map_unicode_open[10]     = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
    static const char *mag_map_unicode_filled[10]   = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
    static const char mag_map_round_ASCII[10]       = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

    static const float min_magnitude = -1.46f;
    static const float max_magnitude = 7.96f;

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

void render_object_stereo(WINDOW *win, struct object_base *object, struct render_flags *rf)
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

    bool use_color = rf->color && object->color_pair != 0;

    if (use_color)
    {
        wattron(win, COLOR_PAIR(object->color_pair));
    }

    // Draw object
    if (rf->unicode)
    {
        mvwaddstr(win, y, x, object->symbol_unicode);
    }
    else
    {
        mvwaddch(win, y, x, object->symbol_ASCII);
    }

    // Draw label
    // FIXME: labels wrap around side, cause flickering
    if (object->label != NULL)
    {
        mvwaddstr(win, y - 1, x + 1, object->label);
    }

    if (use_color)
    {
        wattroff(win, COLOR_PAIR(object->color_pair));
    }
}

void render_stars_stereo(WINDOW *win, struct render_flags *rf,
                         struct star *star_table, int num_stars,
                         int *num_by_mag, float threshold)
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

        render_object_stereo(win, &star->base, rf);
    }

    return;
}

void render_constells(WINDOW *win, struct render_flags *rf,
                      int **constell_table, int num_const,
                      struct star *star_table)
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
            // FIXME: this is too nested
            double line_length = sqrt(pow(ya - yb, 2) + pow(xa - xb, 2));
            if (line_length < 10000)
            {
                if (rf->unicode)
                {
                    draw_line_smooth(win, ya, xa, yb, xb);
                    mvwaddstr(win, ya, xa, "○");
                    mvwaddstr(win, yb, xb, "○");
                }
                else
                {
                    draw_line_ASCII(win, ya, xa, yb, xb);
                    mvwaddch(win, ya, xa, '+');
                    mvwaddch(win, yb, xb, '+');
                }
            }
        }
    }
}

struct planet *generate_planet_table(const struct kep_elems *planet_elements,
                                     const struct kep_rates *planet_rates,
                                     const struct kep_extra *planet_extras)
{
    static const char *planet_symbols_unicode[NUM_PLANETS] =
        {
            [SUN] = "☉",
            [MERCURY] = "☿",
            [VENUS] = "♀",
            [EARTH] = "🜨",
            [MARS] = "♂",
            [JUPITER] = "♃",
            [SATURN] = "♄",
            [URANUS] = "⛢",
            [NEPTUNE] = "♆"};

    static const char planet_symbols_ASCII[NUM_PLANETS] =
        {
            [SUN] = '@',
            [MERCURY] = '*',
            [VENUS] = '*',
            [EARTH] = '*',
            [MARS] = '*',
            [JUPITER] = '*',
            [SATURN] = '*',
            [URANUS] = '*',
            [NEPTUNE] = '*'};

    static const char *planet_labels[NUM_PLANETS] =
        {
            [SUN] = "Sun",
            [MERCURY] = "Mercury",
            [VENUS] = "Venus",
            [EARTH] = "Earth",
            [MARS] = "Mars",
            [JUPITER] = "Jupiter",
            [SATURN] = "Saturn",
            [URANUS] = "Uranus",
            [NEPTUNE] = "Neptune"};

    // TODO: find better way to map these values
    static const int planet_colors[NUM_PLANETS] =
        {
            [SUN] = 4,
            [MERCURY] = 8,
            [VENUS] = 4,
            [MARS] = 2,
            [JUPITER] = 6,
            [SATURN] = 4,
            [URANUS] = 7,
            [NEPTUNE] = 5,
    };

    struct planet *planet_table = malloc(NUM_PLANETS * sizeof(struct planet));

    for (int i = 0; i < NUM_PLANETS; ++i)
    {
        struct planet planet_data;
        planet_data.base = (struct object_base)
        {
            .symbol_ASCII   =           planet_symbols_ASCII[i],
            .symbol_unicode = (char *)  planet_symbols_unicode[i],
            .label          = (char *)  planet_labels[i],
            .color_pair     =           planet_colors[i],
         };
        planet_data.elements    = &planet_elements[i];
        planet_data.rates       = &planet_rates[i];
        planet_data.extras      = NULL;

        if (JUPITER <= i && i <= NEPTUNE)
        {
            planet_data.extras  = &planet_extras[i];
        }

        planet_table[i] = planet_data;
    }

    return planet_table;
}

// FIXME: weird ghost star right where "Earth" would be rendered
void update_planet_positions(struct planet *planet_table, double julian_date,
                             double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    for (int i = SUN; i < NUM_PLANETS; ++i)
    {
        // Geocentric rectangular equatorial coordinates
        double xg, yg, zg;

        // Heliocentric coordinates of the Earth-Moon barycenter
        double xe, ye, ze;
        calc_planet_helio_ICRF(planet_table[EARTH].elements,
                               planet_table[EARTH].rates, planet_table[EARTH].extras,
                               julian_date, &xe, &ye, &ze);

        if (i == SUN)
        {
            // Since the origin of the ICRF frame is the barycenter of the Solar
            // System, (for our purposes this is roughly the position of the Sun)
            // we obtain the geocentric coordinates of the Sun by negating the
            // heliocentric coordinates of the Earth
            xg = -xe;
            yg = -ye;
            zg = -ze;
        }
        else 
        {
            calc_planet_helio_ICRF(planet_table[i].elements,
                                   planet_table[i].rates, planet_table[i].extras,
                                   julian_date, &xg, &yg, &zg);
            
            // Obtain geocentric coordinates by subtracting Earth's coordinates
            xg -= xe;
            yg -= ye;
            zg -= ze;
        }

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

void render_planets_stereo(WINDOW *win, struct render_flags *rf, struct planet *planet_table)
{
    // Render planets so that closest are drawn on top
    for (int i = NUM_PLANETS - 1; i >= 0; --i)
    {
        // Skip rendering the Earth--we're on the Earth! The geocentric
        // coordinates of the Earth are (0.0, 0.0, 0.0) and plotting the "Earth"
        // simply traces along the ecliptic at the approximate hour angle
        if (i == EARTH)
        {
            continue;
        }

        struct planet planet_data = planet_table[i];
        render_object_stereo(win, &planet_data.base, rf);

    }

    return;
}

struct moon generate_moon_object(const struct kep_elems *moon_elements,
                                 const struct kep_rates *moon_rates)
{
    struct moon moon_data;
    moon_data.base      = (struct object_base) {
        .symbol_ASCII   = 'M',
        .symbol_unicode = "🌝︎︎",
        .label          = "Moon",
        .color_pair     = 0,
    };
    moon_data.elements  = moon_elements;
    moon_data.rates     = moon_rates;

    return moon_data;
}

void update_moon_position(struct moon *moon_object, double julian_date,
                          double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    double xg, yg, zg;
    calc_moon_geo_ICRF(moon_object->elements, moon_object->rates, julian_date,
                       &xg, &yg, &zg);

    // Convert to spherical equatorial coordinates
    double right_ascension, declination;
    equatorial_rectangular_to_spherical(xg, yg, zg,
                                        &right_ascension, &declination);

    double azimuth, altitude;
    equatorial_to_horizontal(right_ascension, declination,
                             gmst, latitude, longitude,
                             &azimuth, &altitude);

    moon_object->base.azimuth = azimuth;
    moon_object->base.altitude = altitude;

    return;
}

void update_moon_phase(struct planet *planet_table, struct moon *moon_object,
                       double julian_date)
{
    // Heliocentric coordinates of the Earth-Moon barycenter
    double xe, ye, ze;
    calc_planet_helio_ICRF(planet_table[EARTH].elements,
                           planet_table[EARTH].rates, planet_table[EARTH].extras,
                           julian_date, &xe, &ye, &ze);

    // Convert to geocentric coordinates of the sun
    xe *= -1;
    ye *= -1;
    ze *= -1;

    // Sun's geocentric ecliptic longitude
    double sun_ecliptic_long =  atan2(ye, xe);

    double d = julian_date - 2451544.5;
    double O = moon_object->elements->O + moon_object->rates->dO * d;
    double w = moon_object->elements->w + moon_object->rates->dw * d;
    double M = moon_object->elements->M + moon_object->rates->dM * d;

    // Moon's mean longitude
    double moon_true_long = O + w + M;

    static const char *moon_phases[8] = {"🌑︎", "🌒︎", "🌓︎", "🌔︎", "🌕︎", "🌖︎", "🌗︎", "🌘︎"};
    double phase = calc_moon_phase(sun_ecliptic_long, moon_true_long);
    int phase_index = map_float_to_int_range(0.0, 1.0, 0, 7, phase);
    char *moon_char = (char *) moon_phases[phase_index];

    moon_object->base.symbol_unicode = moon_char;

    return; 
}

void render_moon_stereo(WINDOW *win, struct render_flags *rf, struct moon moon_object)
{
    render_object_stereo(win, &moon_object.base, rf);

    return;
}

int gcd(int a, int b)
{
    int temp;
    while (b != 0)
    {
        temp = a % b;

        a = b;
        b = temp;
    }
    return a;
}

int compare_angles(const void *a, const void *b)
{
    int x = *(int *) a;
    int y = *(int *) b;
    return (90 / gcd(x, 90)) < (90 / gcd(y, 90));
}

void render_azimuthal_grid(WINDOW *win, struct render_flags *rf)
{
    const double to_rad = M_PI / 180.0;

    int rad_vertical = round(win->_maxy / 2.0);
    int rad_horizontal = round(win->_maxx / 2.0);

    // Possible step sizes in degrees (multiples of 5 and factors of 90)
    int step_sizes[5] = {10, 15, 30, 45, 90};
    int length = sizeof(step_sizes) / sizeof(step_sizes[0]);

    // Minumum number of rows separating grid line (at end of window)
    int min_height = 10;

    // Set the step size to the smallest desirable increment
    int inc;
    for (int i = length - 1; i >= 0; --i)
    {
        inc = step_sizes[i];
        if (round(rad_vertical * sin(inc * to_rad)) < min_height)
        {
            inc = step_sizes[--i]; // Go back to previous increment
            break;
        }
    }

    // Sort grid angles in the first quadrant by rendering priority
    int number_angles = 90 / inc + 1;
    int *angles = malloc(number_angles * sizeof(int));

    for (int i = 0; i < number_angles; ++i)
    {
        angles[i] = inc * i;
    }
    qsort(angles, number_angles, sizeof(int), compare_angles);

    // Draw angles in all four quadrants
    for (int quad = 0; quad < 4; ++quad)
    {
        for (int i = 0; i < number_angles; ++i)
        {
            int angle = angles[i] + 90 * quad;

            int y = rad_vertical - round(rad_vertical * sin(angle * to_rad));
            int x = rad_horizontal + round(rad_horizontal * cos(angle * to_rad));

            draw_line_smooth(win, y, x, rad_vertical, rad_horizontal);

            int str_len = snprintf(NULL, 0, "%d", angle);
            char *label = malloc(str_len + 1);

            snprintf(label, str_len + 1, "%d", angle);

            // Offset to avoid truncating string
            int y_off = (y < rad_vertical) ? 1 : -1;
            int x_off = (x < rad_horizontal) ? 0 : -(str_len - 1);

            mvwaddstr(win, y, x + x_off, label);

            free(label);
        }
    }

    // while (angle <= 90.0)
    // {
    //     int rad_x = rad_horizontal * angle / 90.0;
    //     int rad_x = rad_vertical * angle / 90.0;
    //     // draw_ellipse(win, win->_maxy/2, win->_maxx/2, 20, 20, unicode_flag);
    //     angle += inc;
    // }
}

void render_cardinal_directions(WINDOW *win, struct render_flags *rf)
{
    // Render horizon directions

    if (rf->color)
    {
        wattron(win, COLOR_PAIR(5));
    }

    mvwaddch(win, 0, win->_maxx / 2, 'N');
    mvwaddch(win, win->_maxy /2, win->_maxx, 'W');
    mvwaddch(win, win->_maxy, win->_maxx / 2, 'S');
    mvwaddch(win, win->_maxy / 2, 0, 'E');

    if (rf->color)
    {
        wattroff(win, COLOR_PAIR(5));
    }
}