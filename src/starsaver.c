#include "starsaver.h"

#include "astro.h"
#include "term.h"
#include "drawing.h"
#include "coord.h"
#include "parse_BSC5.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
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
    return min_int + (int) round((max_int - min_int) * percent);
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

bool generate_star_table(struct star **star_table_out, struct entry *entries,
                         struct star_name *name_table, unsigned int num_stars)
{
    *star_table_out = malloc(num_stars * sizeof(struct star));
    if (*star_table_out == NULL)
    {
        printf("Allocation of memory for star table failed\n");
        return false;
    }

    for (unsigned int i = 0; i < num_stars; ++i)
    {
        struct star temp_star;

        temp_star.catalog_number    = (int)     entries[i].XNO;
        temp_star.right_ascension   =           entries[i].SRA0;
        temp_star.declination       =           entries[i].SDEC0;
        temp_star.ra_motion         = (double)  entries[i].XRPM;
        temp_star.ra_motion         = (double)  entries[i].XDPM;
        temp_star.magnitude         =           entries[i].MAG / 100.0f;

        // Star magnitude mapping
        const char *mag_map_unicode_round[10] = {"⬤", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅", "⋅"};
        // const char *mag_map_unicode_diamond[10] = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
        // const char *mag_map_unicode_open[10]    = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
        // const char *mag_map_unicode_filled[10]  = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
        const char mag_map_round_ASCII[10] = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

        const float min_magnitude = -1.46f;
        const float max_magnitude = 7.96f;

        int symbol_index = map_float_to_int_range(min_magnitude, max_magnitude,
                                                  0, 9, temp_star.magnitude);

        temp_star.base = (struct object_base)
        {
            .color_pair     = 0,
            .symbol_ASCII   = (char) mag_map_round_ASCII[symbol_index],
        };

        // Allocate memory for unicode symbol
        const char *unicode_temp = mag_map_unicode_round[symbol_index];

        temp_star.base.symbol_unicode = malloc(strlen(unicode_temp) + 1);
        if (temp_star.base.symbol_unicode == NULL)
        {
            printf("Allocation of memory for star struct symbol failed\n");
            return false;
        }
        strcpy(temp_star.base.symbol_unicode, unicode_temp);

        // Allocate memory for label
        if (name_table[i].name != NULL)
        {
            char *label_temp = name_table[i].name;

            temp_star.base.label = malloc(strlen(label_temp) + 1);
            if (temp_star.base.label == NULL)
            {
                printf("Allocation of memory for star struct label failed\n");
                return false;
            }
            strcpy(temp_star.base.label, label_temp);
        }

        // Copy temp struct to table index
        (*star_table_out)[i] = temp_star;
    }

    return true;
}

bool star_numbers_by_magnitude(int **num_by_mag, struct star *star_table,
                               unsigned int num_stars)
{
    // Create and sort a copy of the star table
    struct star *table_copy = malloc(num_stars * sizeof(struct star));
    if (table_copy == NULL)
    {
        printf("Allocation of memory for star table copy failed\n");
        return false;
    }

    memcpy(table_copy, star_table, num_stars * sizeof(*table_copy));
    qsort(table_copy, num_stars, sizeof(struct star), star_magnitude_comparator);

    // Create and fill array of indicies in table copy
    *num_by_mag = malloc(num_stars * sizeof(int));
    if (num_by_mag == NULL)
    {
        printf("Allocation of memory for num by mag array  failed\n");
        return false;
    }

    for (unsigned int i = 0; i < num_stars; ++i)
    {
        (*num_by_mag)[i] = table_copy[i].catalog_number;
    }

    free(table_copy);

    return true;
}

bool generate_name_table(struct star_name **name_table_out, const char *file_path,
                         int num_stars)
{
    *name_table_out = malloc(num_stars * sizeof(struct star_name));
    if (*name_table_out == NULL)
    {
        printf("Allocation of memory for name table failed\n");
        return false;
    }

    FILE *stream;
    stream = fopen(file_path, "r");
    if (stream == NULL)
    {
        printf("Couldn't open file '%s'\n", file_path);
        return false;
    }

    const unsigned BUF_SIZE = 32; // More than enough room to store any line
    char buffer[BUF_SIZE];

    // Fill array with NULL pointers to start
    for (int i = 0; i < num_stars; ++i)
    {
        (*name_table_out)[i].name = NULL;
    }

    // Set desired indicies with names
    while (fgets(buffer, BUF_SIZE, stream))
    {
        // Split by delimiter
        int catalog_number = atoi(strtok(buffer, ","));
        char *name = strtok(NULL, ",\n");

        int table_index = catalog_number - 1;

        struct star_name temp_name;
        temp_name.name = malloc(strlen(name) + 1);
        if (temp_name.name == NULL)
        {
            return false;
        }
        strcpy(temp_name.name, name);

        (*name_table_out)[table_index] = temp_name;
    }

    // Close file
    if (fclose(stream) == EOF)
    {
        printf("Couldn't open file '%s'\n", file_path);
        return false;
    }

    return true;
}

unsigned int count_lines(FILE *file_pointer)
{
    const unsigned BUF_SIZE = 65536;

    char buffer[BUF_SIZE];
    unsigned int count = 0;

    while (fgets(buffer, sizeof(buffer), file_pointer))
    {
        count++;
    }

    return count;
}

bool generate_constell_table(struct constell **constell_table_out, const char *file_path,
                             unsigned int *num_constell_out)
{
    FILE *stream;
    size_t stream_items; // Number of characters read from stream

    stream = fopen(file_path, "r");
    if (stream == NULL)
    {
        printf("Couldn't open file '%s'\n", file_path);
        return false;
    }

    unsigned int num_constells = count_lines(stream);

    rewind(stream); // Reset file pointer position

    *constell_table_out = malloc(num_constells * sizeof(struct constell));
    if (*constell_table_out == NULL)
    {
        printf("Allocation of memory for constellation table failed\n");
        return false;
    }

    const unsigned BUF_SIZE = 256;
    char buffer[BUF_SIZE];

    int i = 0;
    while (fgets(buffer, BUF_SIZE, stream))
    {
        char *name = strtok(buffer, " ");
        int num_segments = atoi(strtok(NULL, " \n"));

        struct constell temp_constell =
        {
            .num_segments = num_segments,
            .star_numbers = NULL,
        };

        // Allocate memory for stars in the constellation
        temp_constell.star_numbers = malloc(num_segments * 2 * sizeof(int));
        if (temp_constell.star_numbers == NULL)
        {
            printf("Allocation of memory for constellation struct failed\n");
            return false;
        }

        int j = 0;
        char *token;
        while ( (token = strtok(NULL, " \n")) )
        {
            temp_constell.star_numbers[j] = atoi(token);
            ++j;
        }

        (*constell_table_out)[i] = temp_constell;
        ++i;
    }

    // Close file
    if (fclose(stream) == EOF)
    {
        printf("Failed to close file '%s'\n", file_path);
        return false;
    }

    *num_constell_out = num_constells;

    return true;
}

// Memory freeing functions

static void free_base_members(struct object_base base)
{
    if (base.symbol_unicode != NULL)
    {
        free(base.symbol_unicode);
    }
    if (base.label != NULL)
    {
        free(base.label);
    }
    return;
}

void free_constell_members(struct constell constell_data)
{
    if (constell_data.star_numbers != NULL)
    {
        free(constell_data.star_numbers);
    }
    return;
}

void free_star_name_members(struct star_name name_data)
{
    if (name_data.name != NULL)
    {
        free(name_data.name);
    }
    return;
}

void free_stars(struct star *star_table, unsigned int size)
{
    for (unsigned int i = 0; i < size; ++i)
    {
        free_base_members(star_table[i].base);
    }
    free(star_table);
    return;
}

void free_planets(struct planet *planets, unsigned int size)
{
    for (unsigned int i = 0; i < size; ++i)
    {
        free_base_members(planets[i].base);
    }
    free(planets);
    return;
}

void free_moon_object(struct moon moon_data)
{
    // Nothing was allocated during moon generation
    return;
}

void free_constells(struct constell *constell_table, unsigned int size)
{
    for (unsigned int i = 0; i < size; i++)
    {
        free_constell_members(constell_table[i]);
    }
    free(constell_table);
    return;
}

void free_star_names(struct star_name *name_table, unsigned int size)
{
    for (unsigned int i = 0; i < size; ++i)
    {
        free_star_name_members(name_table[i]);
    }
    free(name_table);
    return;
}

void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    int i;
    for (i = 0; i < num_stars; ++i)
    {
        struct star *star = &star_table[i];

        double right_ascension, declination;
        calc_star_position(star->right_ascension, star->ra_motion,
                           star->declination, star->dec_motion,
                           julian_date,
                           &right_ascension, &declination);

        // Convert to horizontal coordinates
        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination,
                                 gmst, latitude, longitude,
                                 &azimuth, &altitude);

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
    int height, width;
    getmaxyx(win, height, width);
    polar_to_win(radius_polar, theta_polar,
                 height, width,
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

    return;
}

void render_stars_stereo(WINDOW *win, struct render_flags *rf,
                         struct star *star_table, int num_stars,
                         int *num_by_mag, float threshold)
{
    int i;
    for (i = 0; i < num_stars; ++i)
    {
        int catalog_num = num_by_mag[i];
        int table_index = catalog_num - 1;

        struct star *star = &star_table[table_index];

        if (star->magnitude > threshold)
        {
            continue;
        }

        // FIXME: this is hacky
        if (star->magnitude > rf->label_thresh)
        {
            star->base.label = NULL;
        }

        render_object_stereo(win, &star->base, rf);
    }

    return;
}

void render_constells(WINDOW *win, struct render_flags *rf,
                      struct constell **constell_table, int num_const,
                      struct star *star_table)
{
    int i;
    for (i = 0; i < num_const; ++i)
    {
        struct constell constellation = (*constell_table)[i];
        unsigned int num_segments = constellation.num_segments;

        unsigned int j;
        for (j = 0; j < num_segments * 2; j += 2)
        {

            int catalog_num_a = constellation.star_numbers[j];
            int catalog_num_b = constellation.star_numbers[j + 1];

            int table_index_a = catalog_num_a - 1;
            int table_index_b = catalog_num_b - 1;

            struct star star_a = star_table[table_index_a];
            struct star star_b = star_table[table_index_b];

            int ya = (int) star_a.base.y;
            int xa = (int) star_a.base.x;
            int yb = (int) star_b.base.y;
            int xb = (int) star_b.base.x;

            // This implies the star is not being drawn due to it's magnitude
            // FIXME: this is hacky...
            if ((ya == 0 && xa == 0) || (yb == 0 && xb == 0))
            {
                continue;
            }

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

bool generate_planet_table(struct planet **planet_table,
                           const struct kep_elems *planet_elements,
                           const struct kep_rates *planet_rates,
                           const struct kep_extra *planet_extras)
{
    *planet_table = malloc(NUM_PLANETS * sizeof(struct planet));
    if (*planet_table == NULL)
    {
        return false;
    }

    const char *planet_symbols_unicode[NUM_PLANETS] =
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

    const char planet_symbols_ASCII[NUM_PLANETS] =
    {
        [SUN]       = '@',
        [MERCURY]   = '*',
        [VENUS]     = '*',
        [EARTH]     = '*',
        [MARS]      = '*',
        [JUPITER]   = '*',
        [SATURN]    = '*',
        [URANUS]    = '*',
        [NEPTUNE]   = '*'
    };

    const char *planet_labels[NUM_PLANETS] =
    {
        [SUN]       = "Sun",
        [MERCURY]   = "Mercury",
        [VENUS]     = "Venus",
        [EARTH]     = "Earth",
        [MARS]      = "Mars",
        [JUPITER]   = "Jupiter",
        [SATURN]    = "Saturn",
        [URANUS]    = "Uranus",
        [NEPTUNE]   = "Neptune"
    };

    // TODO: find better way to map these values
    const int planet_colors[NUM_PLANETS] =
    {
        [SUN]       = 4,
        [MERCURY]   = 8,
        [VENUS]     = 4,
        [MARS]      = 2,
        [JUPITER]   = 6,
        [SATURN]    = 4,
        [URANUS]    = 7,
        [NEPTUNE]   = 5,
    };

    unsigned  int i;
    for (i = 0; i < NUM_PLANETS; ++i)
    {
        struct planet temp_planet;

        temp_planet.base = (struct object_base)
        {
            .symbol_ASCII   =   planet_symbols_ASCII[i],
            .symbol_unicode =   NULL,
            .label          =   NULL,
            .color_pair     =   planet_colors[i],
        };

        char *unicode_temp = (char *) planet_symbols_unicode[i];
        char *label_temp = (char *) planet_labels[i];

        if (unicode_temp != NULL)
        {
            temp_planet.base.symbol_unicode = malloc(strlen(unicode_temp) + 1);
            if (temp_planet.base.symbol_unicode == NULL)
            {
                return false;
            }
            strcpy(temp_planet.base.symbol_unicode, unicode_temp);
        }

        if (label_temp != NULL)
        {
            temp_planet.base.label = malloc(strlen(label_temp) + 1);
            if (temp_planet.base.label == NULL)
            {
                return false;
            }
            strcpy(temp_planet.base.label, label_temp);
        }

        temp_planet.elements    = &planet_elements[i];
        temp_planet.rates       = &planet_rates[i];

        if (JUPITER <= i && i <= NEPTUNE)
        {
            temp_planet.extras = &planet_extras[i];
        }
        else
        {
            temp_planet.extras = NULL;
        }

        (*planet_table)[i] = temp_planet;
    }

    return true;
}

void update_planet_positions(struct planet *planet_table, double julian_date,
                             double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    int i;
    for (i = SUN; i < NUM_PLANETS; ++i)
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
    int i;
    for (i = NUM_PLANETS - 1; i >= 0; --i)
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

bool generate_moon_object(struct moon *moon_data,
                          const struct kep_elems *moon_elements,
                          const struct kep_rates *moon_rates)
{
    moon_data->base = (struct object_base)
    {
        .symbol_ASCII   = 'M',
        .symbol_unicode = "🌝︎︎",
        .label          = "Moon",
        .color_pair     = 0,
    };
    moon_data->elements  = moon_elements;
    moon_data->rates     = moon_rates;

    return true;
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

// FIXME: this does not render the correct phase and angle
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

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int rad_vertical = round(maxy / 2.0);
    int rad_horizontal = round(maxx / 2.0);

    // Possible step sizes in degrees (multiples of 5 and factors of 90)
    int step_sizes[5] = {10, 15, 30, 45, 90};
    int length = sizeof(step_sizes) / sizeof(step_sizes[0]);

    // Minumum number of rows separating grid line (at end of window)
    int min_height = 10;

    // Set the step size to the smallest desirable increment
    int inc;
    int i;
    for (i = length - 1; i >= 0; --i)
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

    // int i;
    for (i = 0; i < number_angles; ++i)
    {
        angles[i] = inc * i;
    }
    qsort(angles, number_angles, sizeof(int), compare_angles);

    // Draw angles in all four quadrants
    int quad;
    for (quad = 0; quad < 4; ++quad)
    {
        int i;
        for (i = 0; i < number_angles; ++i)
        {
            int angle = angles[i] + 90 * quad;

            int y = rad_vertical - round(rad_vertical * sin(angle * to_rad));
            int x = rad_horizontal + round(rad_horizontal * cos(angle * to_rad));

            if (rf->unicode)
            {
                draw_line_smooth(win, y, x, rad_vertical, rad_horizontal);
            }
            else
            {
                draw_line_ASCII(win, y, x, rad_vertical, rad_horizontal);
            }

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

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int half_maxy = round(maxy / 2.0);
    int half_maxx = round(maxx / 2.0);

    mvwaddch(win, 0, half_maxx, 'N');
    mvwaddch(win, half_maxy, width - 1, 'W');
    mvwaddch(win, height - 1, half_maxx, 'S');
    mvwaddch(win, half_maxy, 0, 'E');

    if (rf->color)
    {
        wattroff(win, COLOR_PAIR(5));
    }
}

bool string_to_time(char *string, struct tm *time)
{
    char *pointer = strptime(string, "%Y-%m-%dT%H:%M:%S", time);
    mktime(time);

    if (pointer == NULL)
    {
        return false;
    }

    return true;
}
