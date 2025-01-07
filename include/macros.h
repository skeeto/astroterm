#ifndef MACROS_H
#define MACROS_H

// Replace function names to make MSVC happy

#ifdef _MSC_VER
#define tzname _tzname
#define strncasecmp _strnicmp
#define strdup _strdup
#define tzset _tzset
#define tzname _tzname
#define strncasecmp _strnicmp
#endif // _MSC_VER

// Common mathematical constants and functions

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TO_RAD (M_PI / 180.0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif // MACROS_H
