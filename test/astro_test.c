

#include "unity/unity.h"
#include "../include/astro.h"
#include <time.h>
#include <math.h>

// Tolerance for floating-point comparison
#define EPSILON 0.0001

void setUp(void) {}
void tearDown(void) {}

// -----------------------------------------------------------------------------
// datetime_to_julian_date
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// calc_moon_phase
// -----------------------------------------------------------------------------

#define EPSILON_PHASE 0.05

// Account for wrapping around the 0-1 boundary of moon phase
double circular_distance(double phase1, double phase2) {
    double diff = fabs(phase1 - phase2);
    return fmin(diff, 1.0 - diff);
}

void test_calc_moon_phase(void)
{
    // Got actual phases using: https://www.moongiant.com/phase/3/20/2029/
    // Not sure how accurate that really is

    double date;
    double calculated_phase;
    double expected_phase;
    double distance;

    date = 2451550.1;
    expected_phase = 0.0;
    calculated_phase = calc_moon_phase(date);
    distance = circular_distance(calculated_phase, expected_phase);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_PHASE, 0.0, distance);

    date = 2460645.5;
    expected_phase = 0.0;
    calculated_phase = calc_moon_phase(date);
    distance = circular_distance(calculated_phase, expected_phase);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_PHASE, 0.0, distance);

    date = 2459242.5;
    expected_phase = 0.5;
    calculated_phase = calc_moon_phase(date);
    distance = circular_distance(calculated_phase, expected_phase);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_PHASE, 0.0, distance);

    date = 2466447.5;
    expected_phase = 0.5;
    calculated_phase = calc_moon_phase(date);
    distance = circular_distance(calculated_phase, expected_phase);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_PHASE, 0.0, distance);

    // Moving very fast here:
    // date = 2462215.5;
    // expected_phase = 0.25;
    // calculated_phase = calc_moon_phase(date);
    // distance = circular_distance(calculated_phase, expected_phase);
    // TEST_ASSERT_FLOAT_WITHIN(EPSILON_PHASE, 0.0, distance);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_datetime_to_julian_date);
    RUN_TEST(test_calc_moon_phase);
    return UNITY_END();
}
