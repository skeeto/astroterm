/* Core functions for updating object positions
 */

#ifndef CORE_POSITION_H
#define CORE_POSITION_H

#include "core.h"

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

#endif // CORE_POSITION_H
