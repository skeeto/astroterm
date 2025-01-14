#ifndef STRPTIME_H
#define STRPTIME_H

#include <time.h>

char *strptime(const char *s, const char *format, struct tm *tm);

#endif // STRPTIME_H
