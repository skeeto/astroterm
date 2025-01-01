#include "bsc5_data.h"
#include "bsc5_names.h"
#include "core.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// Initialize data structs
static unsigned int num_stars, num_const;

static struct Entry *BSC5_entries;
static struct StarName *name_table;
static struct Star *star_table;
static int *num_by_mag;

void setUp(void)
{
    parse_entries(bsc5_data, bsc5_data_len, &BSC5_entries, &num_stars);
    generate_name_table(bsc5_names, bsc5_names_len, &name_table, num_stars);
    generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);
}

void tearDown(void)
{
    free(star_table);
    free(name_table);
}

#define EPSILON 0.01

void test_generate_star_table(void)
{
    TEST_ASSERT_NOT_NULL(star_table);

    // Verify first star
    TEST_ASSERT_EQUAL(1, star_table[0].catalog_number);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.023, star_table[0].right_ascension);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.789, star_table[0].declination);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, star_table[0].ra_motion);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, star_table[0].dec_motion);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 6.7, star_table[0].magnitude);

    // Verify start with name
    TEST_ASSERT_EQUAL(7001, star_table[7000].catalog_number);
    TEST_ASSERT_EQUAL_STRING("Vega", star_table[7000].base.label);

    // Verify last star
    int last_index = 9110 - 1;
    TEST_ASSERT_EQUAL(9110, star_table[last_index].catalog_number);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.022267, star_table[last_index].right_ascension);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 1.070134, star_table[last_index].declination);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, star_table[last_index].ra_motion);
    TEST_ASSERT_DOUBLE_WITHIN(EPSILON, 0.0, star_table[last_index].dec_motion);
    TEST_ASSERT_FLOAT_WITHIN(EPSILON, 5.8, star_table[last_index].magnitude);
}

void test_generate_name_table(void)
{
    TEST_ASSERT_NOT_NULL(name_table);
    TEST_ASSERT_EQUAL_STRING("Acamar", name_table[896].name);
    TEST_ASSERT_EQUAL_STRING("Vega", name_table[7000].name);
    TEST_ASSERT_EQUAL_STRING("Wezen", name_table[2692].name);
    TEST_ASSERT_EQUAL_STRING("Zubeneschamali", name_table[5684].name);
}

void test_star_numbers_by_magnitude(void)
{
    TEST_ASSERT_NOT_NULL(num_by_mag);

    // Least bright
    TEST_ASSERT_EQUAL(1894, num_by_mag[0]);
    TEST_ASSERT_EQUAL(365, num_by_mag[1]);
    TEST_ASSERT_EQUAL(3313, num_by_mag[2]);

    // Brightest
    // Access the last elements (brightest stars)
    unsigned int last_index = num_stars - 1; // Assuming num_stars is defined elsewhere

    TEST_ASSERT_EQUAL(2491, num_by_mag[last_index]);
    TEST_ASSERT_EQUAL(2326, num_by_mag[last_index - 1]);
    TEST_ASSERT_EQUAL(5340, num_by_mag[last_index - 2]);

    free(num_by_mag);
}

void test_map_float_to_int_range(void)
{
    int result;

    result = map_float_to_int_range(0.0, 1.0, 0, 100, 0.5);
    TEST_ASSERT_EQUAL(50, result);

    result = map_float_to_int_range(-1.0, 1.0, 0, 10, 0.0);
    TEST_ASSERT_EQUAL(5, result);

    result = map_float_to_int_range(0.0, 10.0, 0, 100, 7.5);
    TEST_ASSERT_EQUAL(75, result);
}

void test_string_to_time(void)
{
    const char *time_string = "2025-01-01T12:34:56";
    struct tm time = {0};

    bool result = string_to_time(time_string, &time);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2025 - 1900, time.tm_year);
    TEST_ASSERT_EQUAL(0, time.tm_mon);
    TEST_ASSERT_EQUAL(1, time.tm_mday);
    TEST_ASSERT_EQUAL(12, time.tm_hour);
    TEST_ASSERT_EQUAL(34, time.tm_min);
    TEST_ASSERT_EQUAL(56, time.tm_sec);
}

void test_elapsed_time_to_components(void)
{
    double elapsed_days = 365.25 + 30 + (6.0 / 24.0) + (15.0 / 1440.0) + (30.0 / 86400.0);
    int years, days, hours, minutes, seconds;

    elapsed_time_to_components(elapsed_days, &years, &days, &hours, &minutes, &seconds);

    TEST_ASSERT_EQUAL(1, years);
    TEST_ASSERT_EQUAL(30, days);
    TEST_ASSERT_EQUAL(6, hours);
    TEST_ASSERT_EQUAL(15, minutes);
    TEST_ASSERT_EQUAL(30, seconds);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_generate_star_table);
    RUN_TEST(test_generate_name_table);
    RUN_TEST(test_star_numbers_by_magnitude);
    RUN_TEST(test_map_float_to_int_range);
    RUN_TEST(test_string_to_time);
    RUN_TEST(test_elapsed_time_to_components);

    return UNITY_END();
}
