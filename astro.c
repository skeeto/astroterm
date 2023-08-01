#include "astro.h"

#include <math.h>
#include <time.h>
#include <stdio.h>
// #include <stdlib.h>

#include "misc.h"
#include "coord.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef AU
    #define AU 149597870.691
#endif

int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct star *p1 = (struct star *)v1;
    const struct star *p2 = (struct star *)v2;

    // Lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
}

void calc_star_position(struct star *star, double julian_date, double gmst,
                        double latitude, double longitude,
                        double *azimuth, double *altitude)
{
    // Correct for parallax

    double J200 = 2451545.0;         // J2000 epoch in julian days
    double days_per_year = 365.2425; // Average number of days per year
    double years_from_epoch = (julian_date - J200) / days_per_year;

    double curr_declination = star->declination +
                              star->dec_motion * years_from_epoch;
    double curr_right_ascension = star->right_ascension +
                                  star->ra_motion * years_from_epoch;

    // Convert to horizontal coordinates

    equatorial_to_horizontal(curr_declination, curr_right_ascension,
                             gmst, latitude, longitude,
                             azimuth, altitude);
}

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

enum solar_objects
{
    SUN = 0,
    MERCURY,
    VENUS,
    EARTH,
    MOON,
    MARS,
    JUPITER,
    SATURN,
    URANUS,
    NEPTUNE,
    NUM_SOLAR
};

struct kep_elems
{
    // Keplerian elements
    double a;   // semi-major axis                  (au)
    double e;   // eccentricity
    double I;   // inclination                      (deg)
    double L;   // mean longitude                   (deg)
    double w;   // longitude of perihelion          (deg)
    double O;   // longitude of the ascending node  (deg)
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

struct extra_elems
{
    double b;
    double c;
    double s;
    double f;
};

static const struct kep_elems keplerian_elements[NUM_SOLAR] =
{
    [MERCURY]   = {0.38709843,      0.20563661,     7.00559432,     252.25166724,   77.45771895,    48.33961819     },
    [VENUS]     = {0.72332102,      0.00676399,     3.39777545,     181.97970850,   131.76755713,   76.67261496     },
    [EARTH]     = {1.00000018,      0.01673163,     -0.00054346,    100.46691572,   102.93005885,   -5.11260389     },
    [MARS]      = {1.52371243,      0.09336511,     1.85181869,     -4.56813164,    -23.91744784,   49.71320984     },
    [JUPITER]   = {5.20248019,      0.04853590,     1.29861416,     34.33479152,    14.27495244,    100.29282654    },
    [SATURN]    = {9.54149883,      0.05550825,     2.49424102,     50.07571329,    92.86136063,    113.63998702    },
    [URANUS]    = {19.18797948,     0.04685740,     0.77298127,     314.20276625,   172.43404441,   73.96250215     },
    [NEPTUNE]   = {30.06952752,     0.00895439,     1.77005520,     304.22289287,   46.68158724,    131.78635853    }
};

static const struct kep_rates keplerian_rates[NUM_SOLAR] =
{
    [MERCURY]   = {0.00000000,      0.00002123,     -0.00590158,    149472.67486623,    0.15940013,     -0.12214182 },
    [VENUS]     = {-0.00000026,     -0.00005107,    0.00043494,     58517.81560260,     0.05679648,     -0.27274174 },
    [EARTH]     = {-0.00000003,     -0.00003661,    -0.01337178,    35999.37306329,     0.31795260,     -0.24123856 },
    [MARS]      = {0.00000097,      0.00009149,     -0.00724757,    19140.29934243,     0.45223625,     -0.26852431 },
    [JUPITER]   = {-0.00002864,     0.00018026,     -0.00322699,    3034.90371757,      0.18199196,     0.13024619  },
    [SATURN]    = {-0.00003065,     -0.00032044,    0.00451969,     1222.11494724,      0.54179478,     -0.25015002 },
    [URANUS]    = {-0.00020455,     -0.00001550,    -0.00180155,    428.49512595,       0.09266985,     0.05739699  },
    [NEPTUNE]   = {0.00006447,      0.00000818,     0.00022400,     218.46515314,       0.01009938,     -0.00606302 }
};

static const struct extra_elems keplerian_extras[NUM_SOLAR] =
{
    [JUPITER]   = {-0.00012452,     0.06064060,     -0.35635438,    38.35125000 },
    [SATURN]    = {0.00025899,      -0.13434469,    0.87320147,     38.35125000 },
    [URANUS]    = {0.00058331,      -0.97731848,    0.17689245,     7.67025000  },
    [NEPTUNE]   = {-0.00041348,     0.68346318,     -0.10162547,    7.67025000  }
};

double solve_kepler(double M, double e, double E)
{
    const double rad = M_PI / 180.0;

    double dM = M - (E - e / rad * sin(E * rad));
    double dE = dM / (1.0 - e * cos(E * rad));

    return dE;
}

/* Return the heliocentric position of a planet in equatorial coordinates
 */
void calc_planet_position(int planet, double julian_date)
{
    struct kep_elems elements   = keplerian_elements[planet];
    struct kep_rates rates      = keplerian_rates[planet];
    struct extra_elems extras   = keplerian_extras[planet];

    // Explanatory Supplement to the Astronomical Almanac: Chapter 8,  Page 340

    const double rad = M_PI / 180.0;

    // 1.

    // Calculate number of centuries past J2000
    double t = (julian_date - 2451545.0) / 36525;

    double a = elements.a + rates.da * t;
    double e = elements.e + rates.de * t;
    double I = elements.I + rates.dI * t;
    double L = elements.L + rates.dL * t;
    double w = elements.w + rates.dw * t;
    double O = elements.O + rates.dO * t;

    // 2.

    double ww = w - O;
    double M = L - w;
    if (JUPITER <= planet && planet <= NEPTUNE)
    {
        double b = extras.b;
        double c = extras.c;
        double s = extras.s;
        double f = extras.f;
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
    const double xecl = (cos(ww) * cos(O) - sin(ww) * sin(O) * cos(I)) * xp + (-sin(ww) * cos(O) - cos(ww) * sin(O) * cos(I)) * yp;
    const double yecl = (cos(ww) * sin(O) + sin(ww) * cos(O) * cos(I)) * xp + (-sin(ww) * sin(O) + cos(ww) * cos(O) * cos(I)) * yp;
    const double zecl = (sin(ww) * sin(I)) * xp + (cos(ww) * sin(I)) * yp;

    // 6.

    const double eps = 23.43928 * rad;  // TODO: need to find obliquity of the ecliptic for any julian date (use earth rotation angle somehow?)

    const double x = xecl;
    const double y = cos(eps) * yecl - sin(eps) * zecl;
    const double z = sin(eps) * yecl + cos(eps) * zecl;

    // Convert to equatorial coordinates
    double right_ascension = atan2 (y , x);
    double declination = atan2(z, sqrt( x * x + y * y));
}
