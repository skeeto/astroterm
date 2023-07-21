/* Astronomy functions and utilities.
 */

#ifndef ASTRO_H
#define ASTRO_H

/* Calculate the earth rotation angle in radians given a julian date.
 * TODO: something may be wrong here!
 */
double earth_rotation_angle_rad(double jd);

/* Calculate the greenwich mean sidereal time in radians given a julian date.
 * TODO: something is wrong here!
 */
double greenwich_mean_sidereal_time_rad(double jd);

#endif