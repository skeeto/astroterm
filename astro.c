#include "astro.h"

#include "coord.h"

#include <math.h>
#include <time.h>
#include <stdio.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef AU
    #define AU 149597870.691
#endif

/* Normalize a radian angle to [0, 2π]
 */
double norm_rad(double rad)
{
    double rem = remainder(rad, 2 * M_PI);
    rem += rem < 0 ? 2 * M_PI : 0;
    return rem;
}

void calc_star_position(double right_ascension, double ra_motion,
                        double declination, double dec_motion,
                        double julian_date, double gmst,
                        double latitude, double longitude,
                        double *ITRF_right_ascension, double *ITRF_declination)
{
    double J200 = 2451545.0;         // J2000 epoch in julian days
    double days_per_year = 365.2425; // Average number of days per year
    double years_from_epoch = (julian_date - J200) / days_per_year;

    *ITRF_right_ascension = right_ascension + ra_motion * years_from_epoch;
    *ITRF_declination = declination + dec_motion * years_from_epoch;
}

/* Note: this is NOT the obliquity of the elliptic. Instead, it is the angle
 * from the celestial intermediate origin to the terrestrial intermediate origin
 * and is a replacement for Greenwich sidereal time
 */
double earth_rotation_angle_rad(double jd)
{
    double t = jd - 2451545.0;
    double d = jd - floor(jd);

    // IERS Technical Note No. 32: 5.4.4 eq. (14);
    double theta = 2 * M_PI * (d + 0.7790572732640 + 0.00273781191135448 * t);

    return theta;
}

double greenwich_mean_sidereal_time_rad(double jd)
{

    // Calculate Julian centuries after J2000
    double t = (jd - 2451545.0) / 36525.0;

    // "Expressions for IAU 2000 precession quantities,"
    // N.Capitaine, P.T.Wallace, and J.Chapront

    // This isn't explicitly stated, but I believe this gives the accumulated
    // precession as described in https://en.wikipedia.org/wiki/Sidereal_time
    double acc_precession_sec = -0.014506 - 4612.156534 * t - 1.3915817 *
                                powf(t, 2) + 0.00000044 * powf(t, 3) +
                                0.000029956 * powf(t, 4) + 0.0000000368 *
                                powf(t, 5);

    // Convert to degrees then radians
    double acc_precession_rad = acc_precession_sec / 3600.0 * 180.0 / M_PI;

    double gmst = earth_rotation_angle_rad(jd) - acc_precession_rad; // eq 42

    return gmst;
}

double datetime_to_julian_date(struct tm *time)
{
    // Convert ISO C tm struct to Gregorian datetime format
    int second  = time->tm_sec;
    int minute  = time->tm_min;
    int hour    = time->tm_hour;
    int day     = time->tm_mday;
    int month   = time->tm_mon + 1;
    int year    = time->tm_year + 1900;

    // https://orbital-mechanics.space/reference/julian-date.html
    int a = (month - 14) / 12;          // eq 436
    int b = 1461 * (year + 4800 + a);   // eq 436
    int c = 367 * (month - 2 - 12 * a); // eq 436
    int e = (year + 4900 + a) / 100;    // eq 436

    int julian_day_num = b / 4 + c / 12 - (3 * e) / 4 + day - 32075; // eq 437

    // Determine fraction of seconds that have passed in one day
    double julian_day_frac = (hour - 12) / 24.0 +
                             minute / (24.0 * 60.0) +
                             second / (24.0 * 60.0 * 60.0);

    return julian_day_num + julian_day_frac;
}

struct tm* julian_date_to_datetime(double julian_date)
{
    // https://orbital-mechanics.space/reference/julian-date.html
    int julian_day_num = floor(julian_date);

    int l = julian_day_num + 68569;
    int n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    int i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    int j = 80 * l / 2447;
    
    int day = l - 2447 * j / 80;
    l = j / 11;
    int month = j + 2 - 12 * l;
    int year = 100 * (n - 49) + i + l;

    double julian_day_frac = julian_date - julian_day_num;

    double hour_d   = julian_day_frac * 24.0 + 12; // add twelve because of weird offset
    double minute_d = (hour_d - floor(hour_d)) * 60.0;
    double second_d = (minute_d - floor(minute_d)) * 60.0;

    // Convert Gregorian datetime format to ISO C tm struct
    struct tm *time;
    time->tm_sec    = floor(second_d);
    time->tm_min    = floor(minute_d);
    time->tm_hour   = floor(hour_d);
    time->tm_mday   = day;
    time->tm_mon    = month - 1;
    time->tm_year   = year - 1900;

    // Adjust all fields to usual range
    mktime(time);

    return time;
}

void sun_position(double a, double e, double i,
                  double l, double w, double m,
                  double julian_date,
                  double *right_ascension, double *declination)
{
    // These formulas use days after 2000 Jan 0.0 UT as a timescale
    double d = julian_date - 2451545.0;

    // Compute obliquity of the ecliptic
    double ecl = (23.4393 - 3.563E-7 * d) * 180.0 / M_PI;

    // Compute the eccentric anomaly
    double E = m + e * sin(m) * (1.0 + e * cos(m));

    // Compute the Sun's distance (r) and its true anomaly (v)

    double xv = cos(E) - e;
    double yv = sqrt(1.0 - e*e) * sin(E);

    double v = atan2(yv, xv);
    double r = sqrt(xv * xv + yv * yv);

    // compute the Sun's true longitude
    double lonsun = v + w;

    // Convert lonsun to ecliptic rectangular geocentric coordinates
    double xs = r * cos(lonsun);
    double ys = r * sin(lonsun);

    // Convert to equatorial, rectangular, geocentric coordinates
    double xe = xs;
    double ye = ys * cos(ecl);
    double ze = ys * sin(ecl);

    // Compute right ascension and declination
    *right_ascension    = atan2(ye, xe);
    *declination        = atan2(ze, sqrt(xe*xe+ye*ye));
}

void planetary_positions(double a, double e, double i,
                         double l, double w, double m,
                         double julian_date)
{
    // https://stjarnhimlen.se/comp/ppcomp.html

    // These formulas use days after 2000 Jan 0.0 UT as a timescale
    double d = julian_date - 2451545.0;

    // Solve Kepler's equation: m = e * sin(E) - E, where we are solving for E,
    // the eccentric anomaly

    // First approximation of E
    double E = m + e * sin(m) * (1.0 + e * cos(m));
    
    // Approximate E until E1 and E0 and sufficiently close
    // Will converge if eccentricity (e) is not too close to 1

    double E0, E1;

    E0 = E;
    E1 = E0 - (E0 - e * sin(E0) - m) / (1 - e * cos(E0));
    
    while (fabs(E1 - E0) > 1.0E-5)
    {
        E0 = E1;
        E1 = E0 - (E0 - e * sin(E0) - m) / (1 - e * cos(E0));
    }

    // Compute planet's distance (r) and true anomaly (v)
    double xv = a * (cos(E) - e);
    double yv = a * (sqrt(1.0 - e * e) * sin(E));

    double v = atan2(yv, xv);
    double r = sqrt(xv * xv + yv * yv);

    // Compute the planet's position in 3-dimensional space
    double xh = r * ( cos(l) * cos(v + w) - sin(l) * sin(v + w) * cos(i) );
    double yh = r * ( sin(l) * cos(v + w) + cos(l) * sin(v + w) * cos(i) );
    double zh = r * ( sin(v + w) * sin(i) );

    // Compute the ecliptic longitude and latitude
    double lonecl = atan2(yh, xh);
    double latecl = atan2(zh, sqrt(xh * xh + yh * yh));

    // Correct for precession

    // Convert heliocentric coordinates to geocentric coordinates
    xh = r * cos(lonecl) * cos(latecl);
    yh = r * sin(lonecl) * cos(latecl);
    zh = r               * sin(latecl);

    // xs = rs * cos(lonsun)
    // ys = rs * sin(lonsun)
    return;
}

void solve_orbit(double a, double e, double i,
                 double l, double w, double L,
                 double julian_date)
{

}

double solve_kepler(double M, double e, double E)
{
    const double rad = M_PI / 180.0;

    double dM = M - (E - e / rad * sin(E * rad));
    double dE = dM / (1.0 - e * cos(E * rad));

    return dE;
}

/* Calculate the heliocentric ICRF position of a planet in rectangular
 * equatorial coordinates
 */
void calc_planet_helio_ICRF(const struct kep_elems *elements, const struct kep_rates *rates,
                            const struct kep_extra *extras, double julian_date,
                            double *xh, double *yh, double *zh)
{
    // Explanatory Supplement to the Astronomical Almanac: Chapter 8,  Page 340

    const double rad = M_PI / 180.0;

    // 1.

    // Calculate number of centuries past J2000
    double t = (julian_date - 2451545.0) / 36525.0;

    double a = elements->a + rates->da * t;
    double e = elements->e + rates->de * t;
    double I = elements->I + rates->dI * t;
    double L = elements->L + rates->dL * t;
    double w = elements->w + rates->dw * t;
    double O = elements->O + rates->dO * t;

    // 2.

    double ww = w - O;
    double M = L - w;
    if (extras != NULL)
    {
        double b = extras->b;
        double c = extras->c;
        double s = extras->s;
        double f = extras->f;
        M = L - w + b * t * t + c * cos(f * t * rad) + s * sin(f * t * rad);
    }

    // 3.

    while (M > 180.0)
    {
        M -= 360.0;
    }

    double e_star = 180.0 / M_PI * e;
    double E = M + e_star * sin(M * rad);

    double dE = 1.0;
    int n = 0;
    while (fabs(dE) > 1E-6 && n < 10)
    {
        dE = solve_kepler(M, e, E);
        E += dE;
        n++;
    }

    // 4.

    const double xp = a * (cos(E * rad) - e);
    const double yp = a * sqrt(1 - e * e) * sin(E * rad);
    const double zp = 0;

    // 5.

    a *= rad; e *= rad; I *= rad; L *= rad; ww *= rad; O *= rad;
    double xecl = (cos(ww) * cos(O) - sin(ww) * sin(O) * cos(I)) * xp + (-sin(ww) * cos(O) - cos(ww) * sin(O) * cos(I)) * yp;
    double yecl = (cos(ww) * sin(O) + sin(ww) * cos(O) * cos(I)) * xp + (-sin(ww) * sin(O) + cos(ww) * cos(O) * cos(I)) * yp;
    double zecl = (sin(ww) * sin(I)) * xp + (cos(ww) * sin(I)) * yp;

    // 6.

    // Obliquity at J2000 in radians
    double eps = 84381.448 / (60.0 * 60.0) * rad;

    *xh = xecl;
    *yh = cos(eps) * yecl - sin(eps) * zecl;
    *zh = sin(eps) * yecl + cos(eps) * zecl;

    return;
}

/* Correct ICRF for polar motion, precession, nutation, frame bias & earth
 * rotation
 *
 * According to ASCOM, *true* apparent coordinates would also correct for other
 * elements but those are beyond the scope of this project
 * https://ascom-standards.org/Help/Developer/html/72A95B28-BBE2-4C7D-BC03-2D6AB324B6F7.htm
 */
void ICRF_to_ITRF(double *x, double *y, double *z)
{
    // TODO: implement concise CIO/CEO based transformations 
    *x = *x;
    *y = *y;
    *z = *z;
}

void calc_planet_geo_ICRF(const struct kep_elems *earth_elements, const struct kep_rates *earth_rates,
                          const struct kep_elems *planet_elements, const struct kep_rates *planet_rates,
                          const struct kep_extra *planet_extras,
                          double julian_date,
                          double *xg, double *yg, double *zg)
{
    // Coordinates of desired planet
    double xh, yh, zh;
    calc_planet_helio_ICRF(planet_elements, planet_rates, planet_extras,
                           julian_date, &xh, &yh, &zh);

    // Coordinates of the Earth-Moon Barycenter
    // TODO: expensive calculation, should be moved outside loop
    double xe, ye, ze;
    calc_planet_helio_ICRF(earth_elements, earth_rates, NULL,
                           julian_date, &xe, &ye, &ze);

    // Obtain geocentric coordinates by subtracting Earth's coordinates
    *xg = xh - xe;
    *yg = yh - ye;
    *zg = zh - ze; 

    return;
}

// Precession quantities

double psi_a(double t)
{
    // Expressions for IAU 2000 precession quantities, N. Capitaine, P.T.Wallace,
    // and J. Chapront

    double psi_a_sec = 5038.7784 * t - 1.07259 * pow(t, 2) -
                       0.001147 * powf(t, 3); // Eq. 6
    double psi_a_rad = psi_a_sec / (60.0 * 60.0) * M_PI / 180.0;
    return psi_a_rad;
}

// Nutation quantities

double delta_psi()
{

}

double delta_epsilon()
{

}
