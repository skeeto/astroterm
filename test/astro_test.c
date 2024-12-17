

#include "unity/unity.h"
#include "../include/astro.h"
#include <time.h>

// Tolerance for floating-point comparison
#define EPSILON 0.0001

void setUp(void) {}
void tearDown(void) {}

/* https://ssd.jpl.nasa.gov/tools/jdc/#/cd
 */
void test_datetime_to_julian_date(void)
{
    struct tm time;

    // Test case 1: January 1, 2000, 12:00 UTC
    time.tm_year = 2000 - 1900;
    time.tm_mon = 0;
    time.tm_mday = 1;
    time.tm_hour = 12;
    time.tm_min = 0;
    time.tm_sec = 0;

    double expected_jd = 2451545.0;
    double result = datetime_to_julian_date(&time);

    TEST_ASSERT_FLOAT_WITHIN(EPSILON, expected_jd, result);

    // Test case 2: December 31, 1999, 00:00 UTC
    time.tm_year = 1999 - 1900;
    time.tm_mon = 11;
    time.tm_mday = 31;
    time.tm_hour = 0;
    time.tm_min = 0;
    time.tm_sec = 0;

    expected_jd = 2451543.5;
    result = datetime_to_julian_date(&time);

    TEST_ASSERT_FLOAT_WITHIN(EPSILON, expected_jd, result);

    // Test case 3: July 20, 1969, 20:17 UTC (Apollo 11 Moon Landing)
    time.tm_year = 1969 - 1900;
    time.tm_mon = 6;
    time.tm_mday = 20;
    time.tm_hour = 20;
    time.tm_min = 17;
    time.tm_sec = 0;

    expected_jd = 2440423.34514;
    result = datetime_to_julian_date(&time);

    TEST_ASSERT_FLOAT_WITHIN(EPSILON, expected_jd, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_datetime_to_julian_date);
    return UNITY_END();
}
