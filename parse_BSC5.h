/* Simple parser for the Yale Bright Star Catalog 5:
 * http://tdc-www.harvard.edu/catalogs/bsc5.html
 *
 * Note that I only exposed the project specific functions.
 */

#ifndef PARSE_BSC5_H
#define PARSE_BSC5_H

/* Parse BSC% star catalog and return array of star structures. The star array
 * must ultimately be freed.
 */
struct star *parse_stars(const char *file_path, int *num_stars_return);

#endif