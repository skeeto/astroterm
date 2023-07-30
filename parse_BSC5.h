/* Simple parser for the Yale Bright Star Catalog 5:
 * http://tdc-www.harvard.edu/catalogs/bsc5.html
 *
 * Note that I only exposed the project specific functions.
 */

#ifndef PARSE_BSC5_H
#define PARSE_BSC5_H

/* Parse BSC5 star catalog and return array of star structures. Stars with
 * catalog number `n` are mapped to index `n-1`.This function allocates memory
 * which should  be freed by the caller.
 */
struct star *parse_stars(const char *file_path, int *num_stars_return);

/* Parse BSC5 names and return an array of names. Stars with catalog number `n`
 * are mapped to index `n-1`. This function allocates memory which should be
 * freed by the caller.
 */
char **parse_BSC5_names(const char *file_path, int num_stars);

/* Parse BSC5 constellations and return a matrix, where each index points to an
 * integer array. The first index of each array dictates the number of star
 * pairs in the rest of the array (each star represented by its catalog number).
 * Each pair of stars represents a line segment within the constellation stick
 * figure. This function allocates memory which should  be freed by the caller.
 */
int **parse_BSC5_constellations(const char *file_path, int *num_const_return);

/* Update the label member of star structs given an array mapping catalog
 * numbers to names. Stars with magnitudes above label_thresh will not have a
 * label set.
 */
void set_star_labels(struct star *star_table, char **name_table, int num_stars,
                     float label_thresh);

/* Return an array of star numbers sorted by increasing magnitude
 */
int *star_numbers_by_magnitude(struct star *star_table, int num_stars);

/* Free memory functions
 */
void free_stars(struct star* arr, int size);
void free_star_names(char ** arr, int size);
void free_constellations(int **arr, int size);

#endif