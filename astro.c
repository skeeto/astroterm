#include <math.h>
#include <time.h>
#include <stdio.h>

#include "misc.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef AU
    #define AU 149597870.691
#endif

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

    // determine fraction of seconds that have passed in one day
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
                  double *declination, double *right_ascension)
{
    // These formulas use days after 2000 Jan 0.0 UT as a timescale
    double d = julian_date - 2451545.0;

    // Compute obliquity of the ecliptic
    double ecl = 23.4393 - 3.563E-7 * d;

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