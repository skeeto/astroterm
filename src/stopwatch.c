#include "stopwatch.h"

#include <time.h>
#include <string.h>

// UNIX headers
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#include <unistd.h>     // Needed for _POSIX_TIMERS definintion & usleep()
#endif

// Windows headers
#if defined(_WIN32)
#include <windows.h>
#endif

int sw_gettime(struct sw_timestamp *stamp)
{
    memset(stamp, 0, sizeof(struct sw_timestamp));

#if defined(_WIN32)
    // Microsoft Windows (32-bit or 64-bit)

    LARGE_INTEGER tick;
    int check = QueryPerformanceCounter(&tick);
    if (check == 0) { return -1; } // QueryPerformanceCounter() returns 0 on failure

    stamp->val.tick_windows = tick;
    stamp->val_member = TICK_WIN;

#elif defined(__APPLE__) && defined(__MACH__)
    // Apple OSX and iOS (Darwin)

    unsigned long long tick = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    if (tick == 0) { return -1; } // clock_gettime_nsec_np() returns 0 on failure

    stamp->val.tick_apple = tick;
    stamp->val_member = TICK_APPLE;

#elif defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
    // Available on some POSIX systems (preferable to gettimeofday() below)

    struct timespec tick;
    int check = clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
    if (check == -1) { return -1; } // clock_gettime() returns -1 on failure

    stamp->val.tick_spec = tick;
    stamp->val_member = TICK_SPEC;

#elif defined(__unix__)
    // Should work almost anywhere on Unix, but is not monotonic

    struct timeval tick;
    int check = gettimeofday(&tick, NULL);
    if (check != 0) { return -1; } // gettimeofday() returns non-zero on failure

    stamp->val.tick_val = tick;
    stamp->val_member = TICK_VAL;

#else

    return -1; // Give up

#endif

    return 0;
}


int sw_timediff_usec(struct sw_timestamp end,
                     struct sw_timestamp begin,
                     unsigned long long *diff)
{
    *diff = 0;

    // Ensure unions have same member set
    if (end.val_member != begin.val_member) { return -1; }

#if defined(_WIN32)
// Microsoft Windows (32-bit or 64-bit)

    LARGE_INTEGER frequency; // ticks per second
    int check = QueryPerformanceFrequency(&frequency);
    if (check == 0) { return -1; } // QueryPerformanceFrequency() returns 0 on failure

    *diff = (unsigned long long) (end.val.tick_win.QuadPart -
                                  begin.val.tick_win.QuadPart)
                                  * 1.0E6 / frequency.QuadPart;

#elif defined(__APPLE__) && defined(__MACH__)
    // Apple OSX and iOS (Darwin)

    *diff = (end.val.tick_apple - begin.val.tick_apple) / 1.0E3;            // ns to us

#elif defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
    // Some POSIX systems

    *diff = (unsigned long long) (end.val.tick_spec.tv_sec -
                                  begin.val.tick_spec.tv_sec) * 1.0E6;      // sec to us
    *diff += (unsigned long long) (end.val.tick_spec.tv_nsec -
                                   begin.val.tick_spec.tv_nsec) / 1.0E3;    // ns to us

#elif defined(__unix__)
    // Almost all Unix systems

    *diff = (unsigned long long) (end.val.tick_val.tv_sec -
                                  begin.val.tick_val.tv_sec) * 1.0E6;       // sec to us
    *diff += (unsigned long long) (end.val.tick_val.tv_usec -
                                   begin.val.tick_val.tv_usec);

#else

    return -1; // Give up

#endif

    return 0;
}


int sw_sleep(unsigned long long microseconds)
{
#if defined(_WIN32)
    // Microsoft Windows (32-bit or 64-bit)

    unsigned long milliseconds = (unsigned long) ((double) microseconds / 1.0E3);
    Sleep(milliseconds);

#else
    // Everything else. usleep() should be fairly portable

    int check = usleep(microseconds);
    if (check == -1) { return -1; } // usleep() returns -1 on failure

#endif

    return 0;
}
