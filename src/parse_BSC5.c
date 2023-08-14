#include "parse_BSC5.h"

#include "bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int header_bytes = 28;
static const int entry_bytes = 32;

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

bool parse_entries(const char *file_path, struct entry **entries_out,
                   int *num_entries_out)
{
    FILE *stream;
    int num;                                // Number of characters read from stream
    stream = fopen(file_path, "rb");
    if (stream == NULL) { return false; }   // Error opening file

    // Read header
    uint8_t header_buffer[header_bytes];
    num = fread(header_buffer, sizeof(header_buffer), 1, stream);
    if (!num) { return false; } // Error reading header

    struct header header_data = parse_header(header_buffer);

    // As defined in http://tdc-www.harvard.edu/catalogs/catalogsb.html,
    // STARN can be negative if coordinates are J2000 (which they are in this catalog)
    int num_entries = abs(header_data.STARN);

    // Read entries
    *entries_out = (struct entry *) malloc(num_entries * sizeof(struct entry));
    uint8_t entry_buffer[entry_bytes];

    for (int i = 0; i < num_entries; i++)
    {
        num = fread(entry_buffer, entry_bytes, 1, stream);
        if (!num) { return false; } // Error reading entry

        (*entries_out)[i] = parse_entry(entry_buffer);
    }

    *num_entries_out = num_entries;

    return true;
}
