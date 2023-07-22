/* Astronomy functions and utilities.
 *
 * References:  https://astrogreg.com/convert_ra_dec_to_alt_az.html
 *              https://en.wikipedia.org/wiki/Sidereal_time
 *              https://observablehq.com/@danleesmith/meeus-solar-position-calculations
 */

#ifndef ASTRO_H
#define ASTRO_H

/* Calculate the earth rotation angle in radians given a julian date.
 * TODO: some angles may need normalizing
 */
double earth_rotation_angle_rad(double jd);

/* Calculate the greenwich mean sidereal time in radians given a julian date.
 * TODO: some angles may need normalizing
 */
double greenwich_mean_sidereal_time_rad(double jd);

/* Get the julian date from a given datetime
 */
double datetime_to_julian_date(int year, int month, int day,
                               int hour, int minute, int second);

#endif