#include <stdint.h>

// note: all functions accept bytes in little-endian order and store
//       the resulting primitive in likewise order

// Signed formats

int16_t format_int16(unsigned int index, uint8_t *buffer)
{
    int16_t result = 0x0;
    for (int i = 0; i < sizeof(int16_t); ++i)
    {
        result = result | (int16_t)buffer[index + i] << (8 * i);
    }
    return result;
}

int32_t format_int32(unsigned int index, uint8_t *buffer)
{
    uint32_t result = 0x0;
    for (int i = 0; i < sizeof(int32_t); ++i)
    {
        result = result | (int32_t)buffer[index + i] << (8 * i);
    }
    return result;
}

int64_t format_int64(unsigned int index, uint8_t *buffer)
{
    uint64_t result = 0x0;
    for (int i = 0; i < sizeof(int64_t); ++i)
    {
        result = result | (int64_t)buffer[index + i] << (8 * i);
    }
    return result;
}

// Unsigned formats

uint16_t format_uint16(unsigned int index, uint8_t *buffer)
{
    uint16_t result = 0x0;
    for (int i = 0; i < sizeof(uint16_t); ++i)
    {
        result = result | (uint16_t)buffer[index + i] << (8 * i);
    }
    return result;
}

uint32_t format_uint32(unsigned int index, uint8_t *buffer)
{
    uint32_t result = 0x0;
    for (int i = 0; i < sizeof(uint32_t); ++i)
    {
        result = result | (uint32_t)buffer[index + i] << (8 * i);
    }
    return result;
}

uint64_t format_uint64(unsigned int index, uint8_t *buffer)
{
    uint64_t result = 0x0;
    for (int i = 0; i < sizeof(uint64_t); ++i)
    {
        result = result | (uint64_t)buffer[index + i] << (8 * i);
    }
    return result;
}

// Floating point formats

float format_float32(unsigned int index, uint8_t *buffer)
{
    float f;
    uint32_t tempInt = format_uint32(index, buffer);

    // TODO: since C89+ enforces char to be a minimum of 8 bits
    // we are guaranteed that float stores >= 4 * 8 bits?
    memcpy(&f, &tempInt, sizeof(float));
    return f;
}

double format_double64(unsigned int index, uint8_t *buffer)
{
    double d;

    uint64_t tempInt = format_uint64(index, buffer);

    memcpy(&d, &tempInt, sizeof(double));
    return d;
}