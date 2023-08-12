/* Core cstar functions
 */

#ifndef CSTAR_H
#define CSTAR_H

#include <ncurses.h>

struct render_flags
{
    bool unicode;
    bool color;
};

// Data structures


// All information pertinent to rendering a celestial body
struct object_base
{
    // Cache of last draw coordinates
    long y;                 // FIXME: using ints breaks things...
    long x;                 // FIXME: using ints breaks things...
    double azimuth;
    double altitude;
    int color_pair;         // 0 indicates no color pair
    char symbol_ASCII;
    char *symbol_unicode;
    char *label;
};

struct star
{
    struct object_base base;
    int catalog_number;
    float magnitude;
    double right_ascension;
    double declination;
    double ra_motion;
    double dec_motion;
};

struct planet
{
    struct object_base base;
    const struct kep_elems *elements;
    const struct kep_rates *rates;
    const struct kep_extra *extras;
};

struct moon
{
    struct object_base base;
    const struct kep_elems *elements;
    const struct kep_rates *rates; 
};


// Data structure generation


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

/* Generate an array of planet structs using data from keplerian_elements.h
 */
struct planet *generate_planet_table(const struct kep_elems *planet_elements,
                                     const struct kep_rates *planet_rates,
                                     const struct kep_extra *planet_extras);

/* Generate a moon struct using data from keplerian_elements.h
 */
struct moon generate_moon_object(const struct kep_elems *moon_elements,
                                 const struct kep_rates *moon_rates);

/* Update the label member of star structs given an array mapping catalog
 * numbers to names. Stars with magnitudes above label_thresh will not have a
 * label set.
 */
void set_star_labels(struct star *star_table, char **name_table, int num_stars,
                     float label_thresh);

/* Comparator for star structs
 */
int star_magnitude_comparator(const void *v1, const void *v2);

/* Return an array of star numbers sorted by increasing magnitude
 */
int *star_numbers_by_magnitude(struct star *star_table, int num_stars);


// Memory freeing


void free_stars(struct star *arr, int size);
void free_star_names(char **arr, int size);
void free_constells(int **arr, int size);
void free_planets(struct planet *planets);


// Position update


/* Update apparent star positions for a given observation time and location by
 * setting the azimuth and altitude of each star struct in an array of star
 * structs
 */
void update_star_positions(struct star *star_table, int num_stars,
                           double julian_date, double latitude, double longitude);

/* Update apparent Sun & planet positions for a given observation time and
 * location by setting the azimuth and altitude of each planet struct in an
 * array of planet structs
 */
void update_planet_positions(struct planet *planet_table,
                             double julian_date, double latitude, double longitude);

/* Update apparent Moon positions for a given observation time and
 * location by setting the azimuth and altitude of a moon struct
 */
void update_moon_position(struct moon *moon_object, double julian_date,
                          double latitude, double longitude);

/* Update the phase of the Moon at a given time by setting the unicode symbol
 * for a moon struct
 */
void update_moon_phase(struct planet *planet_table, struct moon *moon_object,
                       double julian_date);


// Rendering


/* Render stars to the screen using a stereographic projection 
 */
void render_stars_stereo(WINDOW *win, struct render_flags *rf,
                         struct star *star_table, int num_stars,
                         int *num_by_mag, float threshold);

/* Render the Sun and planets to the screen using a stereographic projection
 */
void render_planets_stereo(WINDOW *win, struct render_flags *rf,
                           struct planet *planet_table);

/* Render the Moon to the screen using a stereographic projection
 */
void render_moon_stereo(WINDOW *win, struct render_flags *rf,
                        struct moon moon_object);

/* Render constellations
 */
void render_constells(WINDOW *win, struct render_flags *rf, int **constellation_table, int num_const,
                      struct star *star_table);

/* Render an azimuthal grid on a stereographic projection
 */
void render_azimuthal_grid(WINDOW *win, struct render_flags *rf);

/* Render cardinal direction indicators for the Northern, Eastern, Southern, and
 * Western horizons
 */
void render_cardinal_directions(WINDOW *win, struct render_flags *rf);

#endif  // CSTAR_H