#include "stopwatch.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h> // Needed for _POSIX_TIMERS definintion & usleep()

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

union sw_timestamp sw_gettime(void)
{
    union sw_timestamp stamp;

#if defined(_WIN32) || defined(_WIN64)
    // Microsoft Windows (32-bit or 64-bit)

    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);
    stamp.tick_windows = tick;

#elif defined(__APPLE__) && defined(__MACH__)
    // Apple OSX and iOS (Darwin)

    stamp.tick_apple = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

#elif defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
    // Available on some POSIX systems (preferable to gettimeofday() below)

    struct timespec tick;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    stamp.tick_spec = tick;

# else
    // Should work almost anywhere, but is not monotonic

    struct timeval tick;
    gettimeofday(&tick, NULL);
    stamp.tick_val = tick;

#endif

    return stamp;
}

unsigned long sw_timediff_usec(union sw_timestamp end, union sw_timestamp begin)
{
    unsigned long time_diff;

#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER frequency; // ticks per second
    QueryPerformanceFrequency(&frequency);
    time_diff = (end.tick_windows.QuadPart - begin.tick_windows.QuadPart) * 1.0E6 / frequency.QuadPart;

#elif defined(__APPLE__) && defined(__MACH__)

    time_diff = (end.tick_apple - begin.tick_apple) / 1.0E3;                    // ns to us

#elif defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0

    time_diff = (end.tick_spec.tv_sec - begin.tick_spec.tv_sec) * 1.0E6;        // sec to us
    time_diff += (end.tick_spec.tv_nsec - begin.tick_spec.tv_nsec) / 1.0E3;     // ns to us

#else

    time_diff = (end.tick_val.tv_sec - begin.tick_val.tv_sec) * 1.0E6;          // sec to us
    time_diff += (end.tick_val.tv_usec - begin.tick_val.tv_usec);

#endif

    return time_diff;
}

void sw_sleep(unsigned long microseconds)
{
#if defined(_WIN32) || defined(_WIN64)
    // Microsoft Windows (32-bit or 64-bit)

    unsigned long milliseconds = (unsigned long) ((double) microseconds / 1.0E3);
    Sleep(milliseconds);

#else
    // Everything else. usleep() should be fairly portable

    usleep(microseconds);

#endif
}
