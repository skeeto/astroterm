/* Astronomy functions and utilities.
 *
 * References:  https://astrogreg.com/convert_ra_dec_to_alt_az.html
 *              https://en.wikipedia.org/wiki/Sidereal_time
 *              https://observablehq.com/@danleesmith/meeus-solar-position-calculations
 */

#include <time.h>

#ifndef ASTRO_H
#define ASTRO_H

// All information pertinent to rendering a celestial body
struct object_base
{
    double azimuth;
    double altitude;
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

/* Comparator for star structs
 */
int star_magnitude_comparator(const void *v1, const void *v2);

/* Calculate the relative position of a star
 */
void calc_star_position(struct star *star, double julian_date, double gmst,
                        double latitude, double longitude,
                        double *azimuth, double *altitude);

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
                         double l, double w, double m,
                         double julian_date);

#endif