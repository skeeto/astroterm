/* Internal structures used for representing celestial bodies
 */

#ifndef CELESTIAL_BODIES_H
#define CELESTIAL_BODIES_H

/* Generic celestial body
 */
struct body
{
    double altitude;
    double azimuth;
};

/* Stars contain more information than just a body
 */
struct star
{
    int catalog_number;
    float magnitude;
    double right_ascension;
    double declination;
    double altitude;
    double azimuth;
};

int star_magnitude_comparator(const void *v1, const void *v2);

#endif