/* Core functions for rendering
 */

#ifndef CORE_RENDER_H
#define CORE_RENDER_H

#include "core.h"

#include <ncurses.h>

/* Render stars to the screen using a stereographic projection
 */
void render_stars_stereo(WINDOW *win, struct conf *config, struct star *star_table, int num_stars, int *num_by_mag);

/* Render the Sun and planets to the screen using a stereographic projection
 */
void render_planets_stereo(WINDOW *win, struct conf *config, struct planet *planet_table);

/* Render the Moon to the screen using a stereographic projection
 */
void render_moon_stereo(WINDOW *win, struct conf *config, struct moon moon_object);

/* Render constellations
 */
void render_constells(WINDOW *win, struct conf *config, struct constell **constell_table, int num_const,
                      struct star *star_table);

/* Render an azimuthal grid on a stereographic projection
 */
void render_azimuthal_grid(WINDOW *win, struct conf *config);

/* Render cardinal direction indicators for the Northern, Eastern, Southern, and
 * Western horizons
 */
void render_cardinal_directions(WINDOW *win, struct conf *config);

#endif // CORE_RENDER_H
