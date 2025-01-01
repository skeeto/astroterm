/* Astronomy functions and utilities.
 *
 * References:  https://astrogreg.com/convert_ra_dec_to_alt_az.html
 *              https://en.wikipedia.org/wiki/Sidereal_time
 *              https://observablehq.com/@danleesmith/meeus-solar-position-calculations
 */

#ifndef ASTRO_H
#define ASTRO_H

#include <stdbool.h>
#include <time.h>

// For our purposes, the Sun is treated the same as a planet
enum Planets
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

enum MoonPhase
{
    NEW_MOON = 0,
    WAXING_CRESCENT,
    FIRST_QUARTER,
    WAXING_GIBBOUS,
    FULL_MOON,
    WANING_GIBBOUS,
    LAST_QUARTER,
    WANING_CRESCENT
};

enum ZodiacSign
{
    ARIES = 0,
    TAURUS,
    GEMINI,
    CANCER,
    LEO,
    VIRGO,
    LIBRA,
    SCORPIO,
    SAGITTARIUS,
    CAPRICORN,
    AQUARIUS,
    PISCES,
};

// Keplerian/orbital elements

struct KepElems
{
    // Keplerian elements
    double a; // Semi-major axis                    (au)
    double e; // Eccentricity
    double I; // Inclination                        (deg)
    double M; // Mean anomaly                       (deg)
    double w; // Argument of periapsis              (deg)
    double O; // Longitude of the ascending node    (deg)
};

struct KepRates
{
    // Keplerian rates
    double da; // (au/century)
    double de;
    double dI; // (deg/century)
    double dM; // (deg/century)
    double dw; // (deg/century)
    double dO; // (deg/century)
};

struct KepExtra
{
    double b;
    double c;
    double s;
    double f;
};

// Dates and times

/* Calculate the greenwich mean sidereal time in radians given a julian date.
 * TODO: some angles may need normalizing
 */
double greenwich_mean_sidereal_time_rad(double julian_date);

/* Get the julian date from a given datetime
 */
double datetime_to_julian_date(struct tm *time);

/* Get the julian date from a given datetime
 */
struct tm julian_date_to_datetime(double julian_date);

/* Get the current julian date using system time
 */
double current_julian_date(void);

// Celestial body positioning

/* Calculate the relative position of a star
 */
void calc_star_position(double right_ascension, double ra_motion, double declination, double dec_motion, double julian_date,
                        double *ITRF_right_ascension, double *ITRF_declination);

/* Calculate the heliocentric ICRF position of a planet in rectangular
 * equatorial coordinates
 */
void calc_planet_helio_ICRF(const struct KepElems *elements, const struct KepRates *rates, const struct KepExtra *extras,
                            double julian_date, double *xh, double *yh, double *zh);

/* Calculate the geocentric ICRF position of a planet in rectangular
 * equatorial coordinates
 */
void calc_planet_geo_ICRF(double xe, double ye, double ze, const struct KepElems *planet_elements,
                          const struct KepRates *planet_rates, const struct KepExtra *planet_extras, double julian_date,
                          double *xg, double *yg, double *zg);

/* Calculate the geocentric ICRF position of the Moon in rectangular
 * equatorial coordinates
 */
void calc_moon_geo_ICRF(const struct KepElems *moon_elements, const struct KepRates *moon_rates, double julian_date, double *xg,
                        double *yg, double *zg);

// Miscellaneous

/* Note: this is NOT the obliquity of the elliptic. Instead, it is the angle
 * from the celestial intermediate origin to the terrestrial intermediate origin
 * and is a replacement for Greenwich sidereal time
 */
double earth_rotation_angle_rad(double jd);

/* Convert a Julian date to Gregorian calendar date
 */
void julian_to_gregorian(double jd, int *year, int *month, int *day);

/* Get the zodiac sign of a month and day
 */
const char *get_zodiac_sign(int month, int day);

/* Get the zodiac symbol of a month and day
 */
const char *get_zodiac_symbol(int month, int day);

// -----------------------------------------------------------------------------
// Moon calculations
// -----------------------------------------------------------------------------

/* Calculate the age of the Moon, age ∈ [0, 1], where 0 is a New Moon and
 * 0.5 is a Full Moon. I.e. the normalized age of the moon within the synodic month.
 */
double calc_moon_age(double julian_date);

/* Return the phase of the moon given its age
 */
enum MoonPhase moon_age_to_phase(double age);

/* Return the phase of the Moon as a descriptive string (e.g., "Full Moon",
 * "Waxing Crescent") based on the Moon's age within the synodic month.
 */
const char *get_moon_phase_name(enum MoonPhase phase);

/* Return an emoji representing the phase of the Moon. northern = true if in northern hemisphere, otherwise false;
 */
const char *get_moon_phase_image(enum MoonPhase phase, bool northern);

/* Put an angle in degrees, minutes, seconds format
 */
void decimal_to_dms(double decimal_value, int *degrees, int *minutes, double *seconds);

#endif // ASTRO_H
