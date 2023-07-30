#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astro.h"
#include "misc.h"
#include "bit.h"

static const int header_bytes = 28;
static const int entry_bytes = 32;

struct header
{
    int     STAR0;
    int     STAR1;
    int     STARN;
    int     STNUM;
    bool    MPROP;
    int     NMAG;
    int     NBENT;
};

struct entry
{
    float   XNO;
    double  SRA0;
    double  SDEC0;
    char    IS[2];
    float   MAG;
    double  XRPM;
    float   XDPM;
};

struct header parse_header(uint8_t *buffer)
{
    struct header header_data;

    header_data.STAR0   = bytes_to_int32_LE(&buffer[0]);
    header_data.STAR1   = bytes_to_int32_LE(&buffer[4]);
    header_data.STARN   = bytes_to_int32_LE(&buffer[8]);
    header_data.STNUM   = bytes_to_int32_LE(&buffer[12]);
    header_data.MPROP   = bytes_to_bool32_LE(&buffer[16]);
    header_data.NMAG    = bytes_to_int32_LE(&buffer[20]);
    header_data.NBENT   = bytes_to_int32_LE(&buffer[24]);

    return header_data;
}

struct entry parse_entry(uint8_t *buffer)
{
    struct entry entry_data;

    entry_data.XNO      = bytes_to_float32_LE(&buffer[0]);
    entry_data.SRA0     = bytes_to_double64_LE(&buffer[4]);
    entry_data.SDEC0    = bytes_to_double64_LE(&buffer[12]);
    entry_data.IS[0]    = byte_to_char(buffer[20]);
    entry_data.IS[1]    = byte_to_char(buffer[21]);
    entry_data.MAG      = bytes_to_int16_LE(&buffer[22]);
    entry_data.XRPM     = bytes_to_float32_LE(&buffer[24]);
    entry_data.XDPM     = bytes_to_float32_LE(&buffer[28]);

    return entry_data;
}

struct entry *parse_entries(const char *file_path, int *num_entries_return)
{
    FILE *file_pointer;
    file_pointer = fopen(file_path, "rb");

    // Read header
    uint8_t header_buffer[header_bytes];
    fread(header_buffer, sizeof(header_buffer), 1, file_pointer);
    struct header header_data = parse_header(header_buffer);

    // As defined in http://tdc-www.harvard.edu/catalogs/catalogsb.html,
    // STARN can be negative if coordinates are J2000 (which they are in this catalog)
    int num_entries = abs(header_data.STARN);

    // Read entries
    struct entry *entries = (struct entry *) malloc(num_entries * sizeof(struct entry));
    uint8_t entry_buffer[entry_bytes];

    for (int i = 0; i < num_entries; i++)
    {
        fread(entry_buffer, entry_bytes, 1, file_pointer);
        entries[i] = parse_entry(entry_buffer);
    }

    *num_entries_return = num_entries;
    return entries;
}

// Project specific functions used to convert entries to a more practical format

// Star magnitude mapping
static const char *mag_map_unicode_round[10]    = {"⬤", "●", "⦁", "•", "🞄", "∙", "⋅", "⋅", "⋅", "⋅"};
static const char *mag_map_unicode_diamond[10]  = {"⯁", "◇", "⬥", "⬦", "⬩", "🞘", "🞗", "🞗", "🞗", "🞗"};
static const char *mag_map_unicode_open[10]     = {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char *mag_map_unicode_filled[10]   = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"};
static const char mag_map_round_ASCII[10]       = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

static const float min_magnitude = -1.46f;
static const float max_magnitude = 7.96f;

struct star entry_to_star(struct entry *entry_data)
{
    struct star star_data;

    star_data.catalog_number    = (int)     entry_data->XNO;
    star_data.right_ascension   =           entry_data->SRA0;
    star_data.declination       =           entry_data->SDEC0;
    star_data.ra_motion         = (double)  entry_data->XRPM;
    star_data.ra_motion         = (double)  entry_data->XDPM;
    star_data.magnitude         =           entry_data->MAG / 100.0f;

    // This is necessary to avoid printing garbage data as labels. Rendering 
    // functions check if label is NULL to determine whether to print or not 
    star_data.base.label            = NULL;

    int symbol_index = map_float_to_int_range(min_magnitude, max_magnitude,
                                              0, 9, star_data.magnitude);

    star_data.base.symbol_ASCII     = (char)    mag_map_round_ASCII[symbol_index];
    star_data.base.symbol_unicode   = (char *)  mag_map_unicode_round[symbol_index];

    return star_data;
}

struct star *parse_stars(const char *file_path, int *num_stars_return)
{
    int num_entries;
    struct entry *entries = parse_entries(file_path, &num_entries);

    struct star *star_table = (struct star *) malloc(num_entries * sizeof(struct star));

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

char **parse_BSC5_names(const char *file_path, int num_stars)
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

        name_table[table_index] = (char *) malloc(BUF_SIZE * sizeof(char));
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

int **parse_BSC5_constellations(const char *file_path, int *num_const)
{
    FILE *file_pointer;
    file_pointer = fopen(file_path, "r");

    int num_constellations = count_lines(file_pointer);
    rewind(file_pointer); // Reset file pointer position

    int **constellation_table = (int **) malloc(num_constellations * sizeof(int *));

    const unsigned BUF_SIZE = 256;
    char buffer[BUF_SIZE];

    int i = 0;
    while (fgets(buffer, BUF_SIZE, file_pointer))
    {
        // Parse constellation information

        char *name = strtok(buffer, " ");
        int num_pairs = atoi(strtok(NULL, " \n"));

        constellation_table[i] = malloc((num_pairs * 2 + 1) * sizeof(int));

        // Parse constellation stars

        constellation_table[i][0] = num_pairs;

        int j = 1;
        char* token;
        while (token = strtok(NULL, " \n"))
        {
            constellation_table[i][j] = atoi(token);
            j++;
        }

        i++;
    }

    fclose(file_pointer);

    *num_const = num_constellations;
    return constellation_table;
}

// Memory freeing functions

void free_stars(struct star *arr, int size)
{
    free(arr);
    return;
}

void free_constellations(int **arr, int size)
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