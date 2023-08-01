/* Core cstar functions
 */

#ifndef CSTAR_H
#define CSTAR_H

#include <ncurses.h>

/* Parse BSC5 star catalog and return array of star structures. Stars with
 * catalog number `n` are mapped to index `n-1`.This function allocates memory
 * which should  be freed by the caller.
 */
struct star *generate_star_table(const char *file_path, int *num_stars_return);

/* Parse BSC5 names and return an array of names. Stars with catalog number `n`
 * are mapped to index `n-1`. This function allocates memory which should be
 * freed by the caller.
 */
char **generate_name_table(const char *file_path, int num_stars);

/* Parse BSC5 constellations and return a matrix, where each index points to an
 * integer array. The first index of each array dictates the number of star
 * pairs in the rest of the array (each star represented by its catalog number).
 * Each pair of stars represents a line segment within the constellation stick
 * figure. This function allocates memory which should  be freed by the caller.
 */
int **generate_constell_table(const char *file_path, int *num_const_return);

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
void free_stars(struct star *arr, int size);
void free_star_names(char **arr, int size);
void free_constells(int **arr, int size);

/* Update apparent star positions for a given observation time and location by
 * setting the azimuth and altitude of each star struct in an array of star
 * structs
 */
void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude);

/* Render an azimuthal grid on a stereographic projection
 */
void render_azimuthal_grid(WINDOW *win, bool no_unicode);

/* Render stars to the screen using a stereographic projection 
 */
void render_stars(WINDOW *win, struct star *star_table, int num_stars, int *num_by_mag, float threshold, bool no_unicode);

/* Render constellations
 */
void render_constells(WINDOW *win, int **constellation_table, int num_const, struct star *star_table, bool no_unicode);

#endif  // CSTAR_H