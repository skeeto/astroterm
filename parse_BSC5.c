#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "celestial_bodies.h"
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
    struct entry *entries = (struct entry *)malloc(num_entries * sizeof(struct entry));
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

struct star entry_to_star(struct entry *entry_data)
{
    struct star star_data;

    star_data.catalog_number    = (int) entry_data->XNO;
    star_data.right_ascension   =       entry_data->SRA0;
    star_data.declination       =       entry_data->SDEC0;
    star_data.magnitude         =       entry_data->MAG / 100.0f;

    return star_data;
}

struct star *parse_stars(const char *file_path, int *num_stars_return)
{
    int num_entries;
    struct entry *entries = parse_entries(file_path, &num_entries);

    struct star *stars = (struct star *)malloc(num_entries * sizeof(struct star));

    for (int i = 0; i < num_entries; ++i)
    {
        stars[i] = entry_to_star(&entries[i]);
    }

    free(entries);
    
    *num_stars_return = num_entries;
    return stars;
}