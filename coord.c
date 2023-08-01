#include "coord.h"

#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// CONVERSIONS

void equatorial_to_horizontal(double declination, double right_ascension,
                              double gmst, double latitude, double longitude,
                              double *azimuth, double *altitude)
{
    // Compute the approximate hour angle (*not* corrected for nutation)
    double hour_angle = gmst - longitude - right_ascension;

    *altitude = asin(sin(latitude) * sin(declination) +
                     cos(latitude) * cos(declination) * cos(hour_angle));

    *azimuth = atan2(sin(hour_angle), cos(hour_angle) * sin(latitude) -
                     tan(declination) * cos(latitude));

    return;
}

void horizontal_to_spherical(double azimuth, double altitude,
                             double *point_theta, double *point_phi)
{
    *point_theta = M_PI / 2 - azimuth;
    *point_phi = M_PI / 2 - altitude;
}

// MAP PROJECTIONS

void project_stereographic(double sphere_radius, double point_theta, double point_phi,
                           double center_theta, double center_phi,
                           double *radius_polar, double *theta_polar)
{
    // TODO: check this math

    // Map Projections - A Working Manual By JOHN P.SNYDER
    double c = fabs(center_phi - point_phi);    // angular separation between center & point
    *radius_polar = sphere_radius * tan(c / 2); // eq (21 - 1) - dividing by 2 gives projection onto plane containing equator
    *theta_polar = point_theta - center_theta;  // eq (20 - 2) - Snyder uses different polar coords... what else?
}

void project_stereographic_north(double sphere_radius, double theta_point, double point_phi,
                                 double *radius_polar, double *theta_polar)
{
    // Map Projections - A Working Manual By JOHN P.SNYDER
    double c = fabs(0.0 - point_phi);           // angular separation between center (Φ_north_pole = 0) & point
    *radius_polar = sphere_radius * tan(c / 2); // eq (21 - 1) - dividing by 2 gives projection onto plane containing equator
    *theta_polar = theta_point;                 // eq (20 - 2) - Snyder uses different polar coords
}

void project_stereographic_south(double sphere_radius, double point_theta, double point_phi,
                                 double *radius_polar, double *theta_polar)
{
    // Map Projections - A Working Manual By JOHN P.SNYDER
    double c = fabs(M_PI - point_phi);          // angular separation between center (Φ_south_pole = π) & point
    *radius_polar = sphere_radius * tan(c / 2); // eq (21 - 1) - dividing by 2 gives projection onto plane containing equator
    *theta_polar = point_theta;                 // eq (20 - 2) - Snyder uses different polar coords
}

// SCREEN SPACE MAPPING

void polar_to_win(double r, double theta,
                  int win_height, int win_width,
                  int *row, int *col)
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
    // Treat the view window as a "partial" frustum of a sphere
    // Map object coordinates as a "percentage" of this frustum

    double start_phi = perspective_phi - aov_phi / 2;
    double start_theta = perspective_theta - aov_theta / 2;

    *row = (start_phi - object_phi) / aov_phi * win_height;
    *col = (object_theta - start_theta) / aov_theta * win_width;
}