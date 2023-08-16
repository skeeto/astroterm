/* Core starsaver functions
 */

#ifndef STARSAVER_H
#define STARSAVER_H

#include "parse_BSC5.h"

#include <ncurses.h>

struct render_flags
{
    bool unicode;
    bool color;
    float label_thresh;
};


// Data structures


// All information pertinent to rendering a celestial body
struct object_base
{
    // Cache of last draw coordinates
    int y;
    int x;
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

struct constell
{
    unsigned int num_segments;
    int *star_numbers;
};

struct star_name
{
    char *name;
};


// Data structure generation


/* Fill array of star structures using entries from BSC5 and table of star
 * names. Stars with catalog number `n` are mapped to index `n-1`. This function
 * allocates memory which must be freed by the caller. Returns false upon memory
 * allocation error
 */
bool generate_star_table(struct star **star_table, struct entry *entries,
                         struct star_name *name_table, unsigned int num_stars);

/* Parse BSC5_names.txt and return an array of names. Stars with catalog number `n`
 * are mapped to index `n-1`. This function allocates memory which should be
 * freed by the caller. Returns false upon memory allocation or file IO error
 */
bool generate_name_table(struct star_name **name_table_out, const char *file_path,
                         int num_stars);

/* Parse BSC5_constellations.txt and return an array of constell structs. This
 * function allocates memory which should  be freed by the caller. Returns false
 * upon memory allocation or file IO error
 */
bool generate_constell_table(struct constell **constell_table_out,
                             const char *file_path,
                             unsigned int *num_constell_out);

/* Generate an array of planet structs. This function allocates memory which
 * should  be freed by the caller. Returns false upon memory allocation error
 */
bool generate_planet_table(struct planet **planet_table,
                           const struct kep_elems *planet_elements,
                           const struct kep_rates *planet_rates,
                           const struct kep_extra *planet_extras);

/* Generate a moon struct. Returns false upon error during generation
 */
bool generate_moon_object(struct moon *moon_data,
                          const struct kep_elems *moon_elements,
                          const struct kep_rates *moon_rates);


// Structure utils


/* Comparator for star structs
 */
int star_magnitude_comparator(const void *v1, const void *v2);

/* Return an array of star numbers sorted by increasing magnitude
 */
int *star_numbers_by_magnitude(struct star *star_table, unsigned int num_stars);


// Memory freeing


void free_stars(struct star *star_table, unsigned int size);
void free_star_names(struct star_name *name_table, unsigned int size);
void free_constells(struct constell *constell_table, unsigned int size);
void free_planets(struct planet *planets, unsigned int size);
void free_moon_object(struct moon moon_data);


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
void render_constells(WINDOW *win, struct render_flags *rf,
                      struct constell **constell_table, int num_const,
                      struct star *star_table);

/* Render an azimuthal grid on a stereographic projection
 */
void render_azimuthal_grid(WINDOW *win, struct render_flags *rf);

/* Render cardinal direction indicators for the Northern, Eastern, Southern, and
 * Western horizons
 */
void render_cardinal_directions(WINDOW *win, struct render_flags *rf);


// Miscellaneous


/* Parse a string in format yyy-mm-ddThh:mm:ss to a tm struct. Returns false
 * upon error during conversion
 */
bool string_to_time(char *string, struct tm *time);

#endif  // STARSAVER_H
