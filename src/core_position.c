#include "core_position.h"

#include "astro.h"
#include "coord.h"
#include "core.h"

#include <math.h>

void update_star_positions(struct star *star_table, int num_stars, double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    int i;
    for (i = 0; i < num_stars; ++i)
    {
        struct star *star = &star_table[i];

        double right_ascension, declination;
        calc_star_position(star->right_ascension, star->ra_motion, star->declination, star->dec_motion, julian_date,
                           &right_ascension, &declination);

        // Convert to horizontal coordinates
        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination, gmst, latitude, longitude, &azimuth, &altitude);

        star->base.azimuth = azimuth;
        star->base.altitude = altitude;
    }

    return;
}

void update_planet_positions(struct planet *planet_table, double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    int i;
    for (i = SUN; i < NUM_PLANETS; ++i)
    {
        // Geocentric rectangular equatorial coordinates
        double xg, yg, zg;

        // Heliocentric coordinates of the Earth-Moon barycenter
        double xe, ye, ze;
        calc_planet_helio_ICRF(planet_table[EARTH].elements, planet_table[EARTH].rates, planet_table[EARTH].extras, julian_date,
                               &xe, &ye, &ze);

        if (i == SUN)
        {
            // Since the origin of the ICRF frame is the barycenter of the Solar
            // System, (for our purposes this is roughly the position of the
            // Sun) we obtain the geocentric coordinates of the Sun by negating
            // the heliocentric coordinates of the Earth
            xg = -xe;
            yg = -ye;
            zg = -ze;
        }
        else
        {
            calc_planet_helio_ICRF(planet_table[i].elements, planet_table[i].rates, planet_table[i].extras, julian_date, &xg,
                                   &yg, &zg);

            // Obtain geocentric coordinates by subtracting Earth's coordinates
            xg -= xe;
            yg -= ye;
            zg -= ze;
        }

        // Convert to spherical equatorial coordinates
        double right_ascension, declination;
        equatorial_rectangular_to_spherical(xg, yg, zg, &right_ascension, &declination);

        double azimuth, altitude;
        equatorial_to_horizontal(right_ascension, declination, gmst, latitude, longitude, &azimuth, &altitude);

        planet_table[i].base.azimuth = azimuth;
        planet_table[i].base.altitude = altitude;
    }
}

void update_moon_position(struct moon *moon_object, double julian_date, double latitude, double longitude)
{
    double gmst = greenwich_mean_sidereal_time_rad(julian_date);

    double xg, yg, zg;
    calc_moon_geo_ICRF(moon_object->elements, moon_object->rates, julian_date, &xg, &yg, &zg);

    // Convert to spherical equatorial coordinates
    double right_ascension, declination;
    equatorial_rectangular_to_spherical(xg, yg, zg, &right_ascension, &declination);

    double azimuth, altitude;
    equatorial_to_horizontal(right_ascension, declination, gmst, latitude, longitude, &azimuth, &altitude);

    moon_object->base.azimuth = azimuth;
    moon_object->base.altitude = altitude;

    return;
}

// FIXME: this does not render the correct phase and angle
void update_moon_phase(struct moon *moon_object, double julian_date, double latitude)
{
#define NUM_PHASES 8

    // Moon phases throughout the synodic month *as seen from the Northern
    // hemisphere*
    // FIXME: clang-format on CI fails on this line for some reason
    // clang-format off
    static const char *moon_phases[NUM_PHASES] = {"🌑︎", "🌒︎", "🌓︎", "🌔︎", "🌕︎", "🌖︎", "🌗︎", "🌘︎"};
    // clang-format on
    double phase = calc_moon_phase(julian_date);

    // If we are in the Southern hemisphere, negate the phase to move in the
    // opposite direction throughout the cycle
    if (latitude < 0)
    {
        phase = 1 - phase;
    }

    int phase_index = map_float_to_int_range(0.0, 1.0, 0, NUM_PHASES - 1, phase);
    char *moon_char = (char *)moon_phases[phase_index];

    moon_object->base.symbol_unicode = moon_char;

    return;
}
