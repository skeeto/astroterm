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
double datetime_to_julian_date(struct tm *time);

/* Get the julian date from a given datetime
 */
struct tm* julian_date_to_datetime(double julian_date);

/* Orbital elements:
 *
 * a: semi-major axis (mean distance from sun)
 * e: eccentricity
 * i: inclination to the ecliptic
 * l: longitude of ascending node
 * w: argument of perihelion
 * m: mean anomaly
 */
void planetary_positions(double a, double e, double i,
                         double l, double w, double m);

#endif