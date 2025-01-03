/* An attempt at relatively portable & accurate timing library
 *
 * Very helpful reference:
 * https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof
 */

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <time.h>

// UNIX headers
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

struct SwTimestamp
{
    // Timestamp value
    union {
#ifdef _WIN32
        LARGE_INTEGER tick_win;
#endif
        unsigned long long tick_apple;
        struct timespec tick_spec;
        struct timeval tick_val;
    } val;

    // Holds most recently set union member as a sanity check
    enum
    {
        TICK_WIN,
        TICK_APPLE,
        TICK_SPEC,
        TICK_VAL,
    } val_member;
};

/* Set a timestamp. Returns 0 on success and -1 on failure
 */
int sw_gettime(struct SwTimestamp *stamp);

/* Set the difference between two timestamps in microseconds. Returns 0 on
 * success -1 on failure
 */
int sw_timediff_usec(struct SwTimestamp end, struct SwTimestamp begin, unsigned long long *diff);

/* Sleep for the specified number of microseconds. Returns 0 on success and -1
 * on failure
 */
int sw_sleep(unsigned long long microseconds);

#endif // STOPWATCH_H
