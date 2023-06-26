#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>

#include "bin_utils.c"
#include "ascii_utils.c"


struct star
{
    float catalogNumber;
    float magnitude;
    double rightAscension;
    double declination;
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