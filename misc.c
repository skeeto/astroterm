#include <math.h>
#include <stdlib.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

int map_float_to_int_range(double min_float, double max_float,
                           int min_int, int max_int, double input)
{
    double percent = (input - min_float) / (max_float - min_float);
    return min_int + round((max_int - min_int) * percent);
}

double norm_rad(double rad)
{
    double rem = remainder(rad, 2 * M_PI);
    rem += rem < 0 ? 2 * M_PI : 0;
    return rem;
}