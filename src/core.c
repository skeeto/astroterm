#include "core.h"

#include "parse_BSC5.h"
#include "astro.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Count number of lines in file
 */
static unsigned int count_lines(FILE *file_pointer)
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


// Data generation


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

        temp_star.base = (struct object_base){
            .color_pair = 0,
            .symbol_ASCII = (char)mag_map_round_ASCII[symbol_index],
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
            [NEPTUNE]   = "♆"};

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
            [NEPTUNE]   = '*'};

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
            [NEPTUNE]   = "Neptune"};

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

    // TODO: compute these values...?
    const float planet_mean_mags[NUM_PLANETS] =
        {
            [SUN]       = -26.832f,
            [MERCURY]   = 0.23f,
            [VENUS]     = -4.14f,
            [MARS]      = 0.71f,
            [JUPITER]   = -2.20f,
            [SATURN]    = 0.46f,
            [URANUS]    = 5.68f,
            [NEPTUNE]   = 7.78f,
        };

    unsigned int i;
    for (i = 0; i < NUM_PLANETS; ++i)
    {
        struct planet temp_planet;

        temp_planet.base    = (struct object_base){
            .symbol_ASCII   = planet_symbols_ASCII[i],
            .symbol_unicode = NULL,
            .label          = NULL,
            .color_pair     = planet_colors[i],
        };

        char *unicode_temp  = (char *) planet_symbols_unicode[i];
        char *label_temp    = (char *) planet_labels[i];

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
        temp_planet.magnitude   = planet_mean_mags[i];

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

bool generate_moon_object(struct moon *moon_data,
                          const struct kep_elems *moon_elements,
                          const struct kep_rates *moon_rates)
{
    moon_data->base = (struct object_base){
        .symbol_ASCII   = 'M',
        .symbol_unicode = "🌝︎︎",
        .label          = "Moon",
        .color_pair     = 0,
    };

    moon_data->elements = moon_elements;
    moon_data->rates = moon_rates;
    moon_data->magnitude = 0.0f; // TODO: fix this value

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
        while ((token = strtok(NULL, " \n")))
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


// Memory freeing


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


// Miscellaneous


int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct star *p1 = (struct star *) v1;
    const struct star *p2 = (struct star *) v2;

    // Lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
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

int map_float_to_int_range(double min_float, double max_float,
                           int min_int, int max_int, double input)
{
    double percent = (input - min_float) / (max_float - min_float);
    return min_int + (int) round((max_int - min_int) * percent);
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
