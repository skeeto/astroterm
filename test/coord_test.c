#include "coord.h"
#include "math_util.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_project_stereographic_top(void)
{
    // Center top of sphere
    double radius = 1.0;
    double center_theta = 0.0;
    double center_phi = 0.0;

    // Projection straight down should be at origin
    double radius_polar;
    double theta_polar;
    project_stereographic(radius, 0, 0, center_theta, center_phi, &radius_polar, &theta_polar);
    double expected_radius_polar = 0.0;
    double expected_theta_polar = 0.0;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);

    // Projection along equator should be at radius
    project_stereographic(radius, 0, M_PI / 2, center_theta, center_phi, &radius_polar, &theta_polar);
    expected_radius_polar = radius;
    expected_theta_polar = 0.0;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);

    // Projection along equator should be at radius
    project_stereographic(radius, M_PI, M_PI / 2, center_theta, center_phi, &radius_polar, &theta_polar);
    expected_radius_polar = radius;
    expected_theta_polar = M_PI;

    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_radius_polar, radius_polar);
    TEST_ASSERT_FLOAT_WITHIN(0.01, expected_theta_polar, theta_polar);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_project_stereographic_top);

    return UNITY_END();
}
