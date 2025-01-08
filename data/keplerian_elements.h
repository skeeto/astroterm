#ifndef KEP_ELEMS_H
#define KEP_ELEMS_H

#include "astro.h"

// Keplerian elements for orbits of the planets
// https://ssd.jpl.nasa.gov/planets/approx_pos.html *
// *Recomputed to use argument of perihelion & mean anomaly

extern const struct KepElems planet_elements[NUM_PLANETS];
extern const struct KepRates planet_rates[NUM_PLANETS];
extern const struct KepExtra planet_extras[NUM_PLANETS];

// Keplerian elements for orbits of the Moon
// https://stjarnhimlen.se/comp/ppcomp.html#4
// https://ssd.jpl.nasa.gov/planets/approx_pos.html * currently not working

extern const struct KepElems moon_elements;
extern const struct KepRates moon_rates;

#endif // KEP_ELEMS_H
