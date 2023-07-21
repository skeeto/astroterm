#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// CONVERSIONS

void equatorial_to_horizontal(double declination, double right_ascension,
                              double gmst, double latitude, double longitude,
                              double *altitude, double *azimuth)
{
    // TODO: do these angles need to be normalized?
    double hour_angle = gmst - longitude - right_ascension;

    *altitude = asin(sin(latitude) * sin(declination) +
                     cos(latitude) * cos(declination) * cos(hour_angle));

    *azimuth = atan2(sin(hour_angle), cos(hour_angle) * sin(latitude) -
                                          tan(declination) * cos(latitude));
}

void horizontal_to_spherical(double azimuth, double altitude,
                             double *theta_sphere, double *phi_sphere)
{
    *theta_sphere = M_PI / 2 - azimuth;
    *phi_sphere = M_PI / 2 - altitude;
}

// MAP PROJECTIONS

void project_stereographic_south(double radius_sphere, double theta_sphere, double phi_sphere,
                           double *r_polar, double *theta_polar)
{
    // Map Projections - A Working Manual By JOHN P.SNYDER
    *r_polar = radius_sphere * tan(phi_sphere / 2); // eq (21 - 1) modified
    *theta_polar = theta_sphere;                    // eq (20 - 2) modified
}

void project_stereographic_north(double radius_sphere, double theta_sphere, double phi_sphere,
                                 double *r_polar, double *theta_polar)
{
    // Map Projections - A Working Manual By JOHN P.SNYDER
    *r_polar = radius_sphere * tan(M_PI / 2 - phi_sphere / 2);  // eq (21 - 1) modified
    *theta_polar = theta_sphere;                                // eq (20 - 2) modified
}

// SCREEN SPACE MAPPING

void polar_to_win(double r, double theta,
                  int win_height, int win_width,
                  int *row, int *col) // modifies
{
    *row = (int)round(r * win_height / 2 * sin(theta)) + win_height / 2;
    *col = (int)round(r * win_width / 2 * cos(theta)) + win_width / 2;
    return;
}

void perspective_to_win(double aov_phi, double aov_theta,
                        double perspective_phi, double perspective_theta,
                        double object_phi, double object_theta,
                        int win_height, int win_width,
                        int *row, int *col)
{
    // treat the view window as a "partial" frustum of a sphere
    // map object coordinates as a "percentage" of this frustum

    double start_phi = perspective_phi - aov_phi / 2;
    double start_theta = perspective_theta - aov_theta / 2;

    *row = (start_phi - object_phi) / aov_phi * win_height;
    *col = (object_theta - start_theta) / aov_theta * win_width;
}