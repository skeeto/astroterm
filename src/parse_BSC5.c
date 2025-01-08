#include "parse_BSC5.h"

#include "bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_BYTES 28
#define ENTRY_BYTES 32

static struct Header parse_header(uint8_t *buffer)
{
    struct Header header_data;

    header_data.STAR0 = (int)bytes_to_int32_LE(&buffer[0]);
    header_data.STAR1 = (int)bytes_to_int32_LE(&buffer[4]);
    header_data.STARN = (int)bytes_to_int32_LE(&buffer[8]);
    header_data.STNUM = (int)bytes_to_int32_LE(&buffer[12]);
    header_data.MPROP = bytes_to_bool32_LE(&buffer[16]);
    header_data.NMAG = (int)bytes_to_int32_LE(&buffer[20]);
    header_data.NBENT = (int)bytes_to_int32_LE(&buffer[24]);

    return header_data;
}

static struct Entry parse_entry(uint8_t *buffer)
{
    struct Entry entry_data;

    entry_data.XNO = bytes_to_float32_LE(&buffer[0]);
    entry_data.SRA0 = bytes_to_double64_LE(&buffer[4]);
    entry_data.SDEC0 = bytes_to_double64_LE(&buffer[12]);
    entry_data.IS[0] = byte_to_char(buffer[20]);
    entry_data.IS[1] = byte_to_char(buffer[21]);
    entry_data.MAG = (float)bytes_to_int16_LE(&buffer[22]);
    entry_data.XRPM = bytes_to_float32_LE(&buffer[24]);
    entry_data.XDPM = bytes_to_float32_LE(&buffer[28]);

    return entry_data;
}

bool parse_entries(uint8_t *data, size_t data_size, struct Entry **entries_out, unsigned int *num_entries_out)
{
    // Check if there's enough data to read the header
    if (data_size < HEADER_BYTES)
    {
        printf("Insufficient data size for header\n");
        return false;
    }

    // Read the header from the embedded binary data
    uint8_t header_buffer[HEADER_BYTES];
    memcpy(header_buffer, data, HEADER_BYTES);
    struct Header header_data = parse_header(header_buffer);

    // STARN is negative if coordinates are J2000 (which they are in BSC5)
    // http://tdc-www.harvard.edu/catalogs/catalogsb.html
    unsigned int num_entries = (unsigned int)abs(header_data.STARN);

    // Allocate memory for the entries
    *entries_out = malloc(num_entries * sizeof(struct Entry));
    if (*entries_out == NULL)
    {
        printf("Allocation of memory for BSC5 entries failed\n");
        return false;
    }

    // Move the data pointer to the beginning of the entries section
    data += HEADER_BYTES;
    data_size -= HEADER_BYTES;

    // Read entries from the embedded binary data
    uint8_t entry_buffer[ENTRY_BYTES];
    for (unsigned int i = 0; i < num_entries; ++i)
    {
        if (data_size < ENTRY_BYTES)
        {
            printf("Insufficient data size for entry %u\n", i);
            return false;
        }

        memcpy(entry_buffer, data, ENTRY_BYTES);
        (*entries_out)[i] = parse_entry(entry_buffer);

        // Move the data pointer to the next entry
        data += ENTRY_BYTES;
        data_size -= ENTRY_BYTES;
    }

    // Set the number of entries found
    *num_entries_out = num_entries;

    return true;
}
