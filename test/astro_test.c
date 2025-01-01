#include "astro.h"
#include "unity.h"
#include <math.h>
#include <time.h>

// Tolerance for floating-point comparison
#define EPSILON 0.0001

void setUp(void)
{
}
void tearDown(void)
{
}

// datetime_to_julian_date

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

// greenwich_mean_sidereal_time_rad

// TODO: find proper reference
void test_greenwich_mean_sidereal_time_rad(void)
{
    double jd = 2451545.0; // January 1, 2000, 12:00 UTC
    double expected = 4.89496121282306;
    double result = greenwich_mean_sidereal_time_rad(jd);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, expected, result);
}

// -----------------------------------------------------------------------------
// Zodiac
// -----------------------------------------------------------------------------

// get_zodiac_sign

void test_get_zodiac_sign(void)
{
    // Start of zodiacs
    TEST_ASSERT_EQUAL_STRING("Aries", get_zodiac_sign(3, 21));
    TEST_ASSERT_EQUAL_STRING("Taurus", get_zodiac_sign(4, 20));
    TEST_ASSERT_EQUAL_STRING("Gemini", get_zodiac_sign(5, 21));
    TEST_ASSERT_EQUAL_STRING("Cancer", get_zodiac_sign(6, 21));
    TEST_ASSERT_EQUAL_STRING("Leo", get_zodiac_sign(7, 23));
    TEST_ASSERT_EQUAL_STRING("Virgo", get_zodiac_sign(8, 23));
    TEST_ASSERT_EQUAL_STRING("Libra", get_zodiac_sign(9, 23));
    TEST_ASSERT_EQUAL_STRING("Scorpio", get_zodiac_sign(10, 23));
    TEST_ASSERT_EQUAL_STRING("Sagittarius", get_zodiac_sign(11, 22));
    TEST_ASSERT_EQUAL_STRING("Capricorn", get_zodiac_sign(12, 22));
    TEST_ASSERT_EQUAL_STRING("Aquarius", get_zodiac_sign(1, 20));
    TEST_ASSERT_EQUAL_STRING("Pisces", get_zodiac_sign(2, 19));

    // End of zodiacs
    TEST_ASSERT_EQUAL_STRING("Aries", get_zodiac_sign(4, 19));
    TEST_ASSERT_EQUAL_STRING("Taurus", get_zodiac_sign(5, 20));
    TEST_ASSERT_EQUAL_STRING("Gemini", get_zodiac_sign(6, 20));
    TEST_ASSERT_EQUAL_STRING("Cancer", get_zodiac_sign(7, 22));
    TEST_ASSERT_EQUAL_STRING("Leo", get_zodiac_sign(8, 22));
    TEST_ASSERT_EQUAL_STRING("Virgo", get_zodiac_sign(9, 22));
    TEST_ASSERT_EQUAL_STRING("Libra", get_zodiac_sign(10, 22));
    TEST_ASSERT_EQUAL_STRING("Scorpio", get_zodiac_sign(11, 21));
    TEST_ASSERT_EQUAL_STRING("Sagittarius", get_zodiac_sign(12, 21));
    TEST_ASSERT_EQUAL_STRING("Capricorn", get_zodiac_sign(1, 19));
    TEST_ASSERT_EQUAL_STRING("Aquarius", get_zodiac_sign(2, 18));
    TEST_ASSERT_EQUAL_STRING("Pisces", get_zodiac_sign(3, 20));
}

// get_zodiac_symbol

void test_get_zodiac_symbol(void)
{
    // Start of zodiacs
    TEST_ASSERT_EQUAL_STRING("♈", get_zodiac_symbol(3, 21));
    TEST_ASSERT_EQUAL_STRING("♉", get_zodiac_symbol(4, 20));
    TEST_ASSERT_EQUAL_STRING("♊", get_zodiac_symbol(5, 21));
    TEST_ASSERT_EQUAL_STRING("♋", get_zodiac_symbol(6, 21));
    TEST_ASSERT_EQUAL_STRING("♌", get_zodiac_symbol(7, 23));
    TEST_ASSERT_EQUAL_STRING("♍", get_zodiac_symbol(8, 23));
    TEST_ASSERT_EQUAL_STRING("♎", get_zodiac_symbol(9, 23));
    TEST_ASSERT_EQUAL_STRING("♏", get_zodiac_symbol(10, 23));
    TEST_ASSERT_EQUAL_STRING("♐", get_zodiac_symbol(11, 22));
    TEST_ASSERT_EQUAL_STRING("♑", get_zodiac_symbol(12, 22));
    TEST_ASSERT_EQUAL_STRING("♒", get_zodiac_symbol(1, 20));
    TEST_ASSERT_EQUAL_STRING("♓", get_zodiac_symbol(2, 19));
}

// -----------------------------------------------------------------------------
// Moon
// -----------------------------------------------------------------------------

// calc_moon_age

#define EPSILON_AGE 0.05

// Account for wrapping around the 0-1 boundary of moon phase
double circular_distance(double phase1, double phase2)
{
    double diff = fabs(phase1 - phase2);
    return fmin(diff, 1.0 - diff);
}

void test_calc_moon_age(void)
{
    // Got actual phases using: https://www.moongiant.com/phase/3/20/2029/
    // Not sure how accurate that really is

    double date;
    double calculated_age;
    double expected_age;
    double distance;

    date = 2451550.1;
    expected_age = 0.0;
    calculated_age = calc_moon_age(date);
    distance = circular_distance(calculated_age, expected_age);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_AGE, 0.0, distance);

    date = 2460645.5;
    expected_age = 0.0;
    calculated_age = calc_moon_age(date);
    distance = circular_distance(calculated_age, expected_age);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_AGE, 0.0, distance);

    date = 2459242.5;
    expected_age = 0.5;
    calculated_age = calc_moon_age(date);
    distance = circular_distance(calculated_age, expected_age);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_AGE, 0.0, distance);

    date = 2466447.5;
    expected_age = 0.5;
    calculated_age = calc_moon_age(date);
    distance = circular_distance(calculated_age, expected_age);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON_AGE, 0.0, distance);

    // Moving very fast here:
    // date = 2462215.5;
    // expected_age = 0.25;
    // calculated_age = calc_moon_age(date);
    // distance = circular_distance(calculated_age, expected_age);
    // TEST_ASSERT_FLOAT_WITHIN(EPSILON_age, 0.0, distance);
}

// moon_age_to_phase

void test_moon_age_to_phase(void)
{
    TEST_ASSERT_EQUAL(NEW_MOON, moon_age_to_phase(0.0));
    TEST_ASSERT_EQUAL(WAXING_CRESCENT, moon_age_to_phase(0.1));
    TEST_ASSERT_EQUAL(FIRST_QUARTER, moon_age_to_phase(0.25));
    TEST_ASSERT_EQUAL(WAXING_GIBBOUS, moon_age_to_phase(0.4));
    TEST_ASSERT_EQUAL(FULL_MOON, moon_age_to_phase(0.5));
    TEST_ASSERT_EQUAL(WANING_GIBBOUS, moon_age_to_phase(0.6));
    TEST_ASSERT_EQUAL(LAST_QUARTER, moon_age_to_phase(0.75));
    TEST_ASSERT_EQUAL(WANING_CRESCENT, moon_age_to_phase(0.9));
}

// get_moon_phase_name

void test_get_moon_phase_name(void)
{
    TEST_ASSERT_EQUAL_STRING("New Moon", get_moon_phase_name(NEW_MOON));
    TEST_ASSERT_EQUAL_STRING("Waxing Crescent", get_moon_phase_name(WAXING_CRESCENT));
    TEST_ASSERT_EQUAL_STRING("First Quarter", get_moon_phase_name(FIRST_QUARTER));
    TEST_ASSERT_EQUAL_STRING("Waxing Gibbous", get_moon_phase_name(WAXING_GIBBOUS));
    TEST_ASSERT_EQUAL_STRING("Full Moon", get_moon_phase_name(FULL_MOON));
    TEST_ASSERT_EQUAL_STRING("Waning Gibbous", get_moon_phase_name(WANING_GIBBOUS));
    TEST_ASSERT_EQUAL_STRING("Last Quarter", get_moon_phase_name(LAST_QUARTER));
    TEST_ASSERT_EQUAL_STRING("Waning Crescent", get_moon_phase_name(WANING_CRESCENT));
}

// get_moon_phase_image

void test_get_moon_phase_image(void)
{
    // Northern Hemisphere
    TEST_ASSERT_EQUAL_STRING("🌑", get_moon_phase_image(NEW_MOON, true));
    TEST_ASSERT_EQUAL_STRING("🌒", get_moon_phase_image(WAXING_CRESCENT, true));
    TEST_ASSERT_EQUAL_STRING("🌓", get_moon_phase_image(FIRST_QUARTER, true));
    TEST_ASSERT_EQUAL_STRING("🌔", get_moon_phase_image(WAXING_GIBBOUS, true));
    TEST_ASSERT_EQUAL_STRING("🌕", get_moon_phase_image(FULL_MOON, true));
    TEST_ASSERT_EQUAL_STRING("🌖", get_moon_phase_image(WANING_GIBBOUS, true));
    TEST_ASSERT_EQUAL_STRING("🌗", get_moon_phase_image(LAST_QUARTER, true));
    TEST_ASSERT_EQUAL_STRING("🌘", get_moon_phase_image(WANING_CRESCENT, true));

    // Southern Hemisphere
    TEST_ASSERT_EQUAL_STRING("🌑", get_moon_phase_image(NEW_MOON, false));
    TEST_ASSERT_EQUAL_STRING("🌘", get_moon_phase_image(WAXING_CRESCENT, false));
    TEST_ASSERT_EQUAL_STRING("🌗", get_moon_phase_image(FIRST_QUARTER, false));
    TEST_ASSERT_EQUAL_STRING("🌖", get_moon_phase_image(WAXING_GIBBOUS, false));
    TEST_ASSERT_EQUAL_STRING("🌕", get_moon_phase_image(FULL_MOON, false));
    TEST_ASSERT_EQUAL_STRING("🌔", get_moon_phase_image(WANING_GIBBOUS, false));
    TEST_ASSERT_EQUAL_STRING("🌓", get_moon_phase_image(LAST_QUARTER, false));
    TEST_ASSERT_EQUAL_STRING("🌒", get_moon_phase_image(WANING_CRESCENT, false));
}

// -----------------------------------------------------------------------------
// Miscellaneous
// -----------------------------------------------------------------------------

// decimal_to_dms

void test_decimal_to_dms(void)
{
    int degrees, minutes;
    double seconds;

    decimal_to_dms(123.4567, &degrees, &minutes, &seconds);
    TEST_ASSERT_EQUAL_INT(123, degrees);
    TEST_ASSERT_EQUAL_INT(27, minutes);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 24.12, seconds);

    decimal_to_dms(-45.6789, &degrees, &minutes, &seconds);
    TEST_ASSERT_EQUAL_INT(-45, degrees);
    TEST_ASSERT_EQUAL_INT(40, minutes);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 44.04, seconds);

    decimal_to_dms(0.0, &degrees, &minutes, &seconds);
    TEST_ASSERT_EQUAL_INT(0, degrees);
    TEST_ASSERT_EQUAL_INT(0, minutes);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, seconds);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_datetime_to_julian_date);
    RUN_TEST(test_calc_moon_age);
    RUN_TEST(test_greenwich_mean_sidereal_time_rad);
    RUN_TEST(test_get_zodiac_sign);
    RUN_TEST(test_get_zodiac_symbol);
    RUN_TEST(test_moon_age_to_phase);
    RUN_TEST(test_get_moon_phase_name);
    RUN_TEST(test_get_moon_phase_image);
    RUN_TEST(test_decimal_to_dms);
    return UNITY_END();
}
