/* Miscellaneous and general helper functions.
 */

#ifndef MISC_UTILS_H
#define MISC_UTILS_H

/* Map a double `input` which lies in range [min_float, max_float]
 * to an integer which lies in range [min_int, max_int].
 */
int map_float_to_int_range(double min_float, double max_float,
                       int min_int, int max_int, double input);

/* Normalize a radian angle to [0, 2π]
 */
double norm_rad(double rad);

/* Free an array of pointers of a given size
 */
void free_array(void **arr, int size);

#endif