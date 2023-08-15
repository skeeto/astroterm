/* An attempt at relatively portable & accurate timing library
 * 
 * Very helpful reference: https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof
 */

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

union sw_timestamp
{
#ifdef _WIN32
    LARGE_INTEGER tick_windows;
#endif
    unsigned long long tick_apple;
    struct timespec tick_spec;
    struct timeval tick_val;
};

/* Returns a timestamp
 */
union sw_timestamp sw_gettime(void);

/* Returns the difference between two timestamps in microseconds. On 16 & 32 bit
 * systems, will unsigned long can store over an hour in microseconds before
 * wrapping around. Should be safe in that regard.
 * 
 * TODO: is it safe to assume the unions have the same member set?
 */
unsigned long sw_timediff_usec(union sw_timestamp end, union sw_timestamp begin);

/* Sleep for the specified number of microseconds
 */
void sw_sleep(unsigned long microseconds);

#endif  // STOPWATCH_H
