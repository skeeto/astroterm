#ifndef STRPTIME_H
#define STRPTIME_H

#ifndef HAS_STRPTIME
#include <time.h>

char *strptime(const char *s, const char *format, struct tm *tm);

#endif // HAS_STRPTIME

#endif // STRPTIME_H
