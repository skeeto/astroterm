/* Simple parser for the Yale Bright Star Catalog 5:
 * http://tdc-www.harvard.edu/catalogs/bsc5.html
 */

#ifndef PARSE_BSC5_H
#define PARSE_BSC5_H

#include <stdbool.h>

struct header
{
    int STAR0;
    int STAR1;
    int STARN;
    int STNUM;
    bool MPROP;
    int NMAG;
    int NBENT;
};

struct entry
{
    float XNO;
    double SRA0;
    double SDEC0;
    char IS[2];
    float MAG;
    double XRPM;
    float XDPM;
};

/* Parse BSC5 star catalog and return array of entry structures sorted by
 * increasing catalog number (the default order in the BSC5 file).This function
 * allocates memory which should  be freed by the caller.
 */
struct entry *parse_entries(const char *file_path, int *num_entries_return);

#endif  // PARSE_BSC5_H
