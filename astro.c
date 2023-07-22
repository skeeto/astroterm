#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

double earth_rotation_angle_rad(double jd)
{
    double t = jd - 2451545.0;
    int d = jd - floor(jd);

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