#include <stdint.h>

#ifndef BIT_UTILS_H
#define BIT_UTILS_H

uint16_t format_uint16(unsigned int index, uint8_t *buffer);
uint32_t format_uint32(unsigned int index, uint8_t *buffer);
uint64_t format_uint64(unsigned int index, uint8_t *buffer);
float format_float32(unsigned int index, uint8_t *buffer);
double format_double64(unsigned int index, uint8_t *buffer);

#endif