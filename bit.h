/* Byte formatting utilities.
 */

#include <stdint.h>

#ifndef BIT_UTILS_H
#define BIT_UTILS_H

// Convert little-endian sequence of bytes to specified types

int16_t bytes_to_int16_LE(unsigned int index, uint8_t *buffer);
int32_t bytes_to_int32_LE(unsigned int index, uint8_t *buffer);
int64_t bytes_to_int64_LE(unsigned int index, uint8_t *buffer);

uint16_t bytes_to_uint16_LE(unsigned int index, uint8_t *buffer);
uint32_t bytes_to_uint32_LE(unsigned int index, uint8_t *buffer);
uint64_t bytes_to_uint64_LE(unsigned int index, uint8_t *buffer);

float bytes_to_float32_LE(unsigned int index, uint8_t *buffer);
double bytes_to_double64_LE(unsigned int index, uint8_t *buffer);

#endif