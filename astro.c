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

    remainder(theta, 2 * M_PI);
    theta += theta < 0 ? 2 * M_PI : 0;

    return theta;
}

double greenwich_mean_sidereal_time_rad(double jd)
{

    // calculate Julian centuries after J2000
    double t = (jd - 2451545.0) / 36525.0;

    // "Expressions for IAU 2000 precession quantities,"
    // N.Capitaine, P.T.Wallace, and J.Chapront
    double gmst_seconds = earth_rotation_angle_rad(jd) + 0.014506 + 4612.156534 * t +
                          1.3915817 * powf(t, 2) - 0.00000044 * powf(t, 3) -
                          0.000029956 * powf(t, 4) - 0.0000000368 * powf(t, 5); // eq 42

    // convert to degrees then radians
    double gmst_rad = (gmst_seconds / 3600.0) * M_PI / 180.0;

    // normalize
    remainder(gmst_rad, 2 * M_PI);
    gmst_rad += gmst_rad < 0 ? 2 * M_PI : 0;

    return gmst_rad;
}