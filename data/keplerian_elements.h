#ifndef KEP_ELEMS_H
#define KEP_ELEMS_H

#include "astro.h"

// Keplerian elements for orbits of the planets
// https://ssd.jpl.nasa.gov/planets/approx_pos.html *
// *Recomputed to use argument of perihelion & mean anomaly

extern const struct kep_elems planet_elements[NUM_PLANETS];
extern const struct kep_rates planet_rates[NUM_PLANETS];
extern const struct kep_extra planet_extras[NUM_PLANETS];

// Keplerian elements for orbits of the planets
// https://ssd.jpl.nasa.gov/planets/approx_pos.html *

extern const struct kep_elems moon_elements;
extern const struct kep_rates moon_rates;

#endif // KEP_ELEMS_H