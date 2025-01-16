#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_PRECISION 0.001
#include "bit.h"
#include "unity.c"
#include "src/bit.c"

// Required to test doubles
#define UNITY_INCLUDE_DOUBLE

void setUp(void)
{
}

void tearDown(void)
{
}

void test_byte_to_char(void)
{
    TEST_ASSERT_EQUAL('A', byte_to_char(0x41));
    TEST_ASSERT_EQUAL('z', byte_to_char(0x7A));
    TEST_ASSERT_EQUAL('\n', byte_to_char(0x0A));
}

void test_bytes_to_int16_LE(void)
{
    uint8_t buffer[2] = {0x34, 0x12};
    TEST_ASSERT_EQUAL_INT16(0x1234, bytes_to_int16_LE(buffer));

    buffer[0] = 0xFF;
    buffer[1] = 0x7F;
    TEST_ASSERT_EQUAL_INT16(0x7FFF, bytes_to_int16_LE(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x80;
    TEST_ASSERT_EQUAL_INT16((int16_t)0x8000, bytes_to_int16_LE(buffer));
}

void test_bytes_to_int32_LE(void)
{
    uint8_t buffer[4] = {0x78, 0x56, 0x34, 0x12};
    TEST_ASSERT_EQUAL_INT32(0x12345678, bytes_to_int32_LE(buffer));

    buffer[0] = 0xFF;
    buffer[1] = 0xFF;
    buffer[2] = 0xFF;
    buffer[3] = 0x7F;
    TEST_ASSERT_EQUAL_INT32(0x7FFFFFFF, bytes_to_int32_LE(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x80;
    TEST_ASSERT_EQUAL_INT32((int32_t)0x80000000, bytes_to_int32_LE(buffer));
}

void test_bytes_to_uint16_LE(void)
{
    uint8_t buffer[2] = {0x78, 0x56};
    TEST_ASSERT_EQUAL_UINT16(0x5678, bytes_to_uint16_LE(buffer));
}

void test_bytes_to_uint32_LE(void)
{
    uint8_t buffer[4] = {0x12, 0x34, 0x56, 0x78};
    TEST_ASSERT_EQUAL_UINT32(0x78563412, bytes_to_uint32_LE(buffer));
}

void test_bytes_to_float32_LE(void)
{
    uint8_t buffer[4] = {0x00, 0x00, 0x80, 0x3F}; // 1.0 in IEEE 754
    TEST_ASSERT_EQUAL_FLOAT(1.0f, bytes_to_float32_LE(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00; // 0.0
    TEST_ASSERT_EQUAL_FLOAT(0.0f, bytes_to_float32_LE(buffer));
}

void test_bytes_to_double64_LE(void)
{
    uint8_t buffer[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}; // 1.0 in IEEE 754
    TEST_ASSERT_EQUAL_DOUBLE(1.0, bytes_to_double64_LE(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00; // 0.0
    TEST_ASSERT_EQUAL_DOUBLE(0.0, bytes_to_double64_LE(buffer));
}

void test_bytes_to_bool32_LE(void)
{
    uint8_t buffer[4] = {0x01, 0x00, 0x00, 0x00}; // true
    TEST_ASSERT_TRUE(bytes_to_bool32_LE(buffer));

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00; // false
    TEST_ASSERT_FALSE(bytes_to_bool32_LE(buffer));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_byte_to_char);
    RUN_TEST(test_bytes_to_int16_LE);
    RUN_TEST(test_bytes_to_int32_LE);
    RUN_TEST(test_bytes_to_uint16_LE);
    RUN_TEST(test_bytes_to_uint32_LE);
    RUN_TEST(test_bytes_to_float32_LE);
    RUN_TEST(test_bytes_to_double64_LE);
    RUN_TEST(test_bytes_to_bool32_LE);

    return UNITY_END();
}
