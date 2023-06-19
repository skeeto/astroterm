#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char uint8_t;      // single byte
typedef unsigned short uint16_t;    // two byte unsigned integer
typedef unsigned int uint32_t;      // four byte unsigned integer
                                    // uint64_t already declared in stdint.h

/*
    BSC5 is stored in big endian format???
*/
/*
Why use a binary format as opposed to ASCII or JSON. Binary version is far more condensed and easy to serialize.
Screen saver aims to be lightweight--the binary file is ~5x as small as the smallest available JSON file.
*/

uint16_t format_uint16(unsigned int index, uint8_t *buffer)
{
    uint16_t result = 0x0000;
    for (int i = 0; i < sizeof(uint16_t); ++i)
    {
        result = result | (uint16_t) buffer[index + i] << (8 * i);
    }
    return result;
}


uint32_t format_uint32(unsigned int index, uint8_t *buffer)
{
    uint32_t result = 0x00000000;
    for (int i = 0; i < sizeof(uint32_t); ++i)
    {
        result = result | (uint32_t) buffer[index + i] << (8 * i);
    }
    return result;
}


uint64_t format_uint64(unsigned int index, uint8_t *buffer)
{
    uint64_t result = 0x0000000000000000;
    for (int i = 0; i < sizeof(uint64_t); ++i)
    {
        result = result | (uint64_t)buffer[index + i] << (8 * i);
    }
    return result;
}


float format_float32(unsigned int index, uint8_t *buffer)
{
    float f;
    
    // Effectively reverse endianness
    uint32_t reverse = format_uint32(index, buffer);

    memcpy(&f, &reverse, sizeof(float));
    return f;
}


double format_double64(unsigned int index, uint8_t *buffer)
{
    double d;

    // Effectively reverse endianness
    uint64_t reverse = format_uint64(index, buffer);

    memcpy(&d, &reverse, sizeof(double));
    return d;
}

struct star
{
    float catalogNumber;
    double rightAscension;
    double declination;
    float magnitude;
};

void printStar(struct star *star)
{
    printf("ID#: %f\n", star->catalogNumber);
    printf("RA: %f\n", star->rightAscension);
    printf("DC: %f\n", star->declination);
    printf("Mag: %f\n", star->magnitude);
    return;
}

struct star entryToStar(uint8_t *entry)
{

    struct star starData;

    // BSC5 Entry format
    starData.catalogNumber = format_float32(0, entry);
    starData.rightAscension = format_double64(4, entry);
    starData.declination = format_double64(12, entry);
    starData.magnitude = (float)format_uint16(22, entry) / 100;

    return starData;
}

struct star* processDatabase(const char *filePath)
{
    // Read header

    FILE *filePointer;
    filePointer = fopen(filePath, "rb");
    
    // header is defined as 28 bytes
    uint8_t headerBuffer[28];

    fread(headerBuffer, sizeof(headerBuffer), 1, filePointer);

    uint32_t numStars = abs((int) format_uint32(8, headerBuffer)); // We know BSC5 uses J2000 cords.
    uint32_t bytesPerEntry = format_uint32(24, headerBuffer);

    printf("%u\n", bytesPerEntry);

    // Read entries

    // Manually allocate variable sized arrays
    struct star* stars = (struct star *) malloc(numStars * sizeof(struct star));
    uint8_t *entryBuffer = (uint8_t *)malloc( bytesPerEntry);

    for (unsigned int i = 0; i < numStars; i++)
    {
        fread(entryBuffer, bytesPerEntry, 1, filePointer);
        stars[i] = entryToStar(entryBuffer);
    }

    for (unsigned int i = 0; i < numStars; i++)
    {
        printStar(&stars[i]);
    }

    free(entryBuffer);

    return stars;
}

int main()
{
    const char filePath[8] = "BSC5";

    struct star *stars = processDatabase(filePath);

    free(stars);

    puts("Press <enter> to quit:");
    getchar();

    return 0;
}