#include "stopwatch.h"
#include "unity.h"
#include <stdlib.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_sw_gettime_should_return_success(void)
{
    struct SwTimestamp timestamp;
    int result = sw_gettime(&timestamp);
    TEST_ASSERT_EQUAL(0, result);

    // Check that the returned timestamp corresponds to a valid value
    switch (timestamp.val_member)
    {
#ifdef _WIN32
    case TICK_WIN:
        TEST_ASSERT_NOT_EQUAL(0, timestamp.val.tick_win.QuadPart);
        break;
#endif
    case TICK_APPLE:
        TEST_ASSERT_NOT_EQUAL(0, timestamp.val.tick_apple);
        break;
    case TICK_SPEC:
        TEST_ASSERT_NOT_EQUAL(0, timestamp.val.tick_spec.tv_sec);
        break;
    case TICK_VAL:
        TEST_ASSERT_NOT_EQUAL(0, timestamp.val.tick_val.tv_sec);
        break;
    default:
        TEST_FAIL_MESSAGE("Invalid val_member in timestamp");
    }
}

void test_sw_timediff_usec_should_calculate_difference(void)
{
    struct SwTimestamp start, end;
    unsigned long long diff;

    // Record two timestamps
    TEST_ASSERT_EQUAL(0, sw_gettime(&start));
    sw_sleep(100); // Small sleep
    TEST_ASSERT_EQUAL(0, sw_gettime(&end));

    int result = sw_timediff_usec(end, start, &diff);
    TEST_ASSERT_EQUAL(0, result);

    // We can't really expect accurate timing during testing, so we'll just make sure some time actually passed
    TEST_ASSERT_NOT_EQUAL(0, diff);
}

void test_sw_sleep_should_pause_execution(void)
{
    struct SwTimestamp start, end;
    unsigned long long diff;

    TEST_ASSERT_EQUAL(0, sw_gettime(&start));
    sw_sleep(500000); // Sleep for 500 milliseconds
    TEST_ASSERT_EQUAL(0, sw_gettime(&end));

    TEST_ASSERT_EQUAL(0, sw_timediff_usec(end, start, &diff));

    // Ensure the elapsed time is close to 500 milliseconds (500000 microseconds)
    // Allow some margin for error. This value is much larger so we can expect it to be reasonably close. This also helps detect
    // weird overflow issues.
    TEST_ASSERT_UINT_WITHIN(50000, 500000, diff);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_sw_gettime_should_return_success);
    RUN_TEST(test_sw_timediff_usec_should_calculate_difference);
    RUN_TEST(test_sw_sleep_should_pause_execution);

    return UNITY_END();
}
