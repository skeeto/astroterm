#include "unity/unity.h"
#include "../include/coord.h"

void setUp(void)
{
    // Optional: Code to set up the test environment before each test
}

void tearDown(void)
{
    // Optional: Code to clean up after each test
}

void test_addition(void)
{
    TEST_ASSERT_EQUAL(5, 2+3); // Replace with your function and test conditions
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_addition);

    return UNITY_END();
}
