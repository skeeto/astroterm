/* Astronomy functions and utilities.
 *
 * References:  https://astrogreg.com/convert_ra_dec_to_alt_az.html
 *              https://en.wikipedia.org/wiki/Sidereal_time
 *              https://observablehq.com/@danleesmith/meeus-solar-position-calculations
 */

#ifndef ASTRO_H
#define ASTRO_H

#include <time.h>

// For our purposes, the Sun is treated the same as a planet
enum planets
{
    SUN = 0,
    MERCURY,
    VENUS,
    EARTH,
    MARS,
    JUPITER,
    SATURN,
    URANUS,
    NEPTUNE,
    NUM_PLANETS
};

// Data in data/keplerian_elements.h

struct kep_elems
{
    // Keplerian elements
    double a; // semi-major axis                  (au)
    double e; // eccentricity
    double I; // inclination                      (deg)
    double L; // mean longitude                   (deg)
    double w; // longitude of perihelion          (deg)
    double O; // longitude of the ascending node  (deg)
};

struct kep_rates
{
    // Keplerian rates
    double da; // (au/century)
    double de;
    double dI; // (deg/century)
    double dL; // (deg/century)
    double dw; // (deg/century)
    double dO; // (deg/century)
};

struct kep_extra
{
    double b;
    double c;
    double s;
    double f;
};

/* Calculate the relative position of a star
 */
void calc_star_position(double right_ascension, double ra_motion,
                        double declination, double dec_motion,
                        double julian_date, double gmst,
                        double latitude, double longitude,
                        double *ITRF_right_ascension, double *ITRF_declination);

/* Calculate the earth rotation angle in radians given a julian date.
 * TODO: some angles may need normalizing
 */
double earth_rotation_angle_rad(double jd);

/* Calculate the geocentric ICRF position of a planet in rectangular
 * equatorial coordinates
 */
void calc_planet_geo_ICRF(const struct kep_elems *earth_elements, const struct kep_rates *earth_rates,
                          const struct kep_elems *planet_elements, const struct kep_rates *planet_rates,
                          const struct kep_extra *planet_extras,
                          double julian_date,
                          double *xg, double *yg, double *zg);

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

#endif  // ASTRO_H