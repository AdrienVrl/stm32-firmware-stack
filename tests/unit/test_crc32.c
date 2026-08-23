#include "crc32.h"
#include "stdint.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_empty_input(void)
{
    uint32_t crc = crc32_init();
    TEST_ASSERT_EQUAL_HEX32(0x00000000u, crc32_finalize(crc));
}

void test_zero_length_update_is_noop(void)
{
    uint32_t crc   = crc32_init();
    uint32_t after = crc32_update(crc, (const uint8_t *)"ignored", 0u);
    TEST_ASSERT_EQUAL_HEX32(crc, after);
}

void test_known_vector_check_string(void)
{
    const uint8_t data[] = "123456789";
    uint32_t crc         = crc32_init();
    crc                  = crc32_update(crc, data, 9u);
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32_finalize(crc));
}

void test_known_vector_pangram(void)
{
    const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
    uint32_t crc         = crc32_init();
    crc                  = crc32_update(crc, data, sizeof(data) - 1u);
    TEST_ASSERT_EQUAL_HEX32(0x414FA339u, crc32_finalize(crc));
}

void test_known_vector_single_byte(void)
{
    const uint8_t data[] = "A";
    uint32_t crc         = crc32_init();
    crc                  = crc32_update(crc, data, 1u);
    TEST_ASSERT_EQUAL_HEX32(0xD3D99E8Bu, crc32_finalize(crc));
}

void test_incremental_matches_one_shot(void)
{
    const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
    size_t len           = sizeof(data) - 1u;

    uint32_t one_shot = crc32_init();
    one_shot          = crc32_update(one_shot, data, len);
    one_shot          = crc32_finalize(one_shot);

    uint32_t chunked = crc32_init();
    chunked          = crc32_update(chunked, data, 10u);
    chunked          = crc32_update(chunked, data + 10u, len - 10u);
    chunked          = crc32_finalize(chunked);

    TEST_ASSERT_EQUAL_HEX32(one_shot, chunked);
}

void test_incremental_byte_at_a_time_matches_one_shot(void)
{
    const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
    size_t len           = sizeof(data) - 1u;
    size_t i;

    uint32_t one_shot = crc32_init();
    one_shot          = crc32_update(one_shot, data, len);
    one_shot          = crc32_finalize(one_shot);

    uint32_t byte_wise = crc32_init();
    for (i = 0; i < len; i++)
    {
        byte_wise = crc32_update(byte_wise, &data[i], 1u);
    }
    byte_wise = crc32_finalize(byte_wise);

    TEST_ASSERT_EQUAL_HEX32(one_shot, byte_wise);
}

void test_different_data_gives_different_crc(void)
{
    const uint8_t a[] = "firmware image v1";
    const uint8_t b[] = "firmware image v2";

    uint32_t crc_a = crc32_finalize(crc32_update(crc32_init(), a, sizeof(a) - 1u));
    uint32_t crc_b = crc32_finalize(crc32_update(crc32_init(), b, sizeof(b) - 1u));

    TEST_ASSERT_NOT_EQUAL(crc_a, crc_b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_input);
    RUN_TEST(test_zero_length_update_is_noop);
    RUN_TEST(test_known_vector_check_string);
    RUN_TEST(test_known_vector_pangram);
    RUN_TEST(test_known_vector_single_byte);
    RUN_TEST(test_incremental_matches_one_shot);
    RUN_TEST(test_incremental_byte_at_a_time_matches_one_shot);
    RUN_TEST(test_different_data_gives_different_crc);
    UNITY_END();
    return 0;
}
