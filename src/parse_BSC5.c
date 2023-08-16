#include "parse_BSC5.h"

#include "bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t header_bytes = 28;
static const size_t entry_bytes = 32;

static struct header parse_header(uint8_t *buffer)
{
    struct header header_data;

    header_data.STAR0   = (int) bytes_to_int32_LE(&buffer[0]);
    header_data.STAR1   = (int) bytes_to_int32_LE(&buffer[4]);
    header_data.STARN   = (int) bytes_to_int32_LE(&buffer[8]);
    header_data.STNUM   = (int) bytes_to_int32_LE(&buffer[12]);
    header_data.MPROP   =       bytes_to_bool32_LE(&buffer[16]);
    header_data.NMAG    = (int) bytes_to_int32_LE(&buffer[20]);
    header_data.NBENT   = (int) bytes_to_int32_LE(&buffer[24]);

    return header_data;
}

static struct entry parse_entry(uint8_t *buffer)
{
    struct entry entry_data;

    entry_data.XNO      =           bytes_to_float32_LE(&buffer[0]);
    entry_data.SRA0     =           bytes_to_double64_LE(&buffer[4]);
    entry_data.SDEC0    =           bytes_to_double64_LE(&buffer[12]);
    entry_data.IS[0]    =           byte_to_char(buffer[20]);
    entry_data.IS[1]    =           byte_to_char(buffer[21]);
    entry_data.MAG      = (float)   bytes_to_int16_LE(&buffer[22]);
    entry_data.XRPM     =           bytes_to_float32_LE(&buffer[24]);
    entry_data.XDPM     =           bytes_to_float32_LE(&buffer[28]);

    return entry_data;
}

bool parse_entries(struct entry **entries_out, const char *file_path,
                   unsigned int *num_entries_out)
{
    FILE *stream;
    size_t stream_items;    // Number of characters read from stream

    stream = fopen(file_path, "rb");
    if (stream == NULL)
    {
        return false;
    }

    // Read header
    uint8_t header_buffer[header_bytes];
    stream_items = fread(header_buffer, sizeof(header_buffer), 1, stream);
    if (stream_items == 0)
    {
        return false;
    }

    struct header header_data = parse_header(header_buffer);

    // STARN is negative if coordinates are J2000 (which they are in BSC5)
    // http://tdc-www.harvard.edu/catalogs/catalogsb.html
    unsigned int num_entries = (unsigned int) abs(header_data.STARN);

    *entries_out = malloc(num_entries * sizeof(struct entry));
    if (*entries_out == NULL)
    {
        return false;
    }

    // Read entriess
    uint8_t entry_buffer[entry_bytes];
    for (unsigned int i = 0; i < num_entries; ++i)
    {
        stream_items = fread(entry_buffer, entry_bytes, 1, stream);
        if (stream_items == 0)
        {
            return false;
        }

        (*entries_out)[i] = parse_entry(entry_buffer);
    }

    // Close file
    if (fclose(stream) == EOF)
    {
        return false;
    }

    *num_entries_out = num_entries;

    return true;
}
