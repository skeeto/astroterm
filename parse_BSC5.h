/* Simple parser for the Yale Bright Star Catalog 5:
 * http://tdc-www.harvard.edu/catalogs/bsc5.html
 *
 * Note that I only exposed the project specific functions.
 */

#ifndef PARSE_BSC5_H
#define PARSE_BSC5_H

/* Parse BSC5 star catalog and return array of star structures. The star array
 * must ultimately be freed
 */
struct star *parse_stars(const char *file_path, int *num_stars_return);

/* Parse BSC5 names and return an array of names, where the index of the name
 * is equivalent to the star's catalog number. The name array must ultimately be
 * freed
 */
char **parse_BSC5_names(const char *file_path, int num_stars);

/* Update the label member of star structs given an array mapping
 * catalog numbers to names. Stars with magnitudes above label_thresh will not
 * have a label set
 */
void set_star_labels(struct star *stars, char **star_names, int num_stars,
                     float label_thresh);

#endif