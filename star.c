#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef unsigned short uint16_t;            // two byte unsigned integer
typedef unsigned int uint32_t;              // four byte unsigned integer
typedef int int32_t;
// uint64_t already declared in stdint.h

struct star
{
    uint32_t catalogNumber;  // not a type???
    uint64_t rightAscension;
    uint64_t declination;
    float magnitude;
};

 
/*
Why use a binary format as opposed to ASCII or JSON. Binary version is far more condensed and easy to serialze.
Screen saver aims to be lightweight--the binary file is ~5x as small as the smallest available JSON file.
*/

struct star entryToStar(uint16_t *entry)
{

    struct star starData;

    // BSC5 Entry format
    starData.catalogNumber =    (uint32_t) *(entry) << 16 |
                                (uint32_t) *(entry + 1);
    starData.rightAscension =   (uint64_t) *(entry + 2) << 48 |
                                (uint64_t) *(entry + 3) << 32 |
                                (uint64_t) *(entry + 4) << 16 |
                                (uint64_t) *(entry + 5);
    starData.declination =      (uint64_t) *(entry + 6) << 48 |
                                (uint64_t) *(entry + 7) << 32 |
                                (uint64_t) *(entry + 8) << 16 |
                                (uint64_t) *(entry + 9);
                                // Don't care about spectral type
    starData.magnitude =        *(entry + 11); // float is 4 bytes--how to convert this?

    return starData;
}

struct star* processDatabase(const char *filePath)
{
    // Read header

    FILE *filePointer;
    filePointer = fopen(filePath, "rb");
    
    // header is defined as 28 bytes
    uint32_t headerBuffer[7];

    fread(headerBuffer, sizeof(headerBuffer), 1, filePointer);

    uint32_t numStars = abs((int32_t) headerBuffer[2]); // signed
    uint32_t bytesPerEntry = headerBuffer[6];

    // Read entries

    struct star* stars = (struct star *) malloc(numStars * sizeof(struct star));
    uint16_t* entryBuffer = (uint16_t *) malloc(bytesPerEntry);

    printf("%d\n", numStars);
    printf("%u\n", numStars);
    printf("%x\n", numStars);

    /*
    for (unsigned int i = 0; i < numStars; i++)
    {
        fread(entryBuffer, sizeof(entryBuffer), 1, filePointer);
        stars[i] = entryToStar(entryBuffer);
        printf("%zu", (unsigned int) stars[i].catalogNumber);
        printf("EEE\n");
    }*/

    free(entryBuffer);

    return stars;
}

uint32_t Combine(unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4)
{
    int combined = b1 << 8 | b2;
    return combined;
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