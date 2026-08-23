#include "stdint.h"
#include "unity.h"
#include "update_protocol.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_decode_u32_le_basic(void)
{
    uint8_t bytes[4] = {0x01, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX32(1u, update_decode_u32_le(bytes));
}

void test_decode_u32_le_byte_order(void)
{
    /* 0x00000100 = 256, little-endian on the wire as {0x00, 0x01, 0x00, 0x00} */
    uint8_t bytes[4] = {0x00, 0x01, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX32(256u, update_decode_u32_le(bytes));
}

void test_decode_u32_le_all_ones(void)
{
    uint8_t bytes[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, update_decode_u32_le(bytes));
}

void test_decode_u32_le_each_byte_lands_in_right_place(void)
{
    uint8_t bytes[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    TEST_ASSERT_EQUAL_HEX32(0xDDCCBBAAu, update_decode_u32_le(bytes));
}

void test_parse_header_splits_size_and_crc(void)
{
    /* image_size = 480 * 1024 = 0x00078000, crc = 0xDEADBEEF */
    uint8_t header[8] = {
        0x00, 0x80, 0x07, 0x00, /* size, LE */
        0xEF, 0xBE, 0xAD, 0xDE, /* crc, LE */
    };
    uint32_t size = 0u;
    uint32_t crc  = 0u;

    update_parse_header(header, &size, &crc);

    TEST_ASSERT_EQUAL_HEX32(480u * 1024u, size);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFu, crc);
}

void test_next_chunk_len_full_chunk_when_plenty_remains(void)
{
    TEST_ASSERT_EQUAL(UPDATE_CHUNK_SIZE, update_next_chunk_len(480u * 1024u, 0u));
}

void test_next_chunk_len_exact_multiple_boundary(void)
{
    /* offset sits exactly one chunk before the end - should still
     * return a full chunk, not zero. */
    uint32_t image_size = UPDATE_CHUNK_SIZE * 4u;
    uint32_t offset     = UPDATE_CHUNK_SIZE * 3u;
    TEST_ASSERT_EQUAL(UPDATE_CHUNK_SIZE, update_next_chunk_len(image_size, offset));
}

void test_next_chunk_len_partial_final_chunk(void)
{
    uint32_t image_size = (UPDATE_CHUNK_SIZE * 3u) + 17u;
    uint32_t offset     = UPDATE_CHUNK_SIZE * 3u;
    TEST_ASSERT_EQUAL(17u, update_next_chunk_len(image_size, offset));
}

void test_next_chunk_len_image_smaller_than_one_chunk(void)
{
    TEST_ASSERT_EQUAL(100u, update_next_chunk_len(100u, 0u));
}

void test_next_chunk_len_nothing_left_returns_zero(void)
{
    uint32_t image_size = UPDATE_CHUNK_SIZE * 2u;
    TEST_ASSERT_EQUAL(0u, update_next_chunk_len(image_size, image_size));
}

void test_next_chunk_len_offset_past_end_returns_zero(void)
{
    TEST_ASSERT_EQUAL(0u, update_next_chunk_len(100u, 500u));
}

void test_is_last_chunk_true_on_exact_end(void)
{
    uint32_t image_size = (UPDATE_CHUNK_SIZE * 2u) + 17u;
    uint32_t offset     = UPDATE_CHUNK_SIZE * 2u;
    TEST_ASSERT_TRUE(update_is_last_chunk(image_size, offset, 17u));
}

void test_is_last_chunk_false_when_more_remains(void)
{
    uint32_t image_size = UPDATE_CHUNK_SIZE * 4u;
    uint32_t offset     = 0u;
    TEST_ASSERT_FALSE(update_is_last_chunk(image_size, offset, UPDATE_CHUNK_SIZE));
}

void test_is_last_chunk_true_for_single_chunk_image(void)
{
    /* Whole image fits in one chunk - that one chunk is the last one. */
    TEST_ASSERT_TRUE(update_is_last_chunk(100u, 0u, 100u));
}

void test_full_transfer_walk_matches_expected_chunk_count(void)
{
    /* Walk update_next_chunk_len()/update_is_last_chunk() the same way
     * update_run_session() does, over a non-round image size, and check
     * it terminates on exactly the last byte with the right chunk count. */
    uint32_t image_size = (UPDATE_CHUNK_SIZE * 3u) + 1u;
    uint32_t offset     = 0u;
    int chunk_count     = 0;
    bool saw_last       = false;

    while (offset < image_size)
    {
        uint32_t len = update_next_chunk_len(image_size, offset);
        TEST_ASSERT_TRUE(len > 0u);

        bool is_last = update_is_last_chunk(image_size, offset, len);
        offset += len;
        chunk_count++;

        if (is_last)
        {
            saw_last = true;
            TEST_ASSERT_EQUAL(image_size, offset);
        }
        else
        {
            TEST_ASSERT_FALSE(offset >= image_size);
        }
    }

    TEST_ASSERT_TRUE(saw_last);
    TEST_ASSERT_EQUAL(4, chunk_count); /* 3 full chunks + 1 one-byte chunk */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_decode_u32_le_basic);
    RUN_TEST(test_decode_u32_le_byte_order);
    RUN_TEST(test_decode_u32_le_all_ones);
    RUN_TEST(test_decode_u32_le_each_byte_lands_in_right_place);
    RUN_TEST(test_parse_header_splits_size_and_crc);
    RUN_TEST(test_next_chunk_len_full_chunk_when_plenty_remains);
    RUN_TEST(test_next_chunk_len_exact_multiple_boundary);
    RUN_TEST(test_next_chunk_len_partial_final_chunk);
    RUN_TEST(test_next_chunk_len_image_smaller_than_one_chunk);
    RUN_TEST(test_next_chunk_len_nothing_left_returns_zero);
    RUN_TEST(test_next_chunk_len_offset_past_end_returns_zero);
    RUN_TEST(test_is_last_chunk_true_on_exact_end);
    RUN_TEST(test_is_last_chunk_false_when_more_remains);
    RUN_TEST(test_is_last_chunk_true_for_single_chunk_image);
    RUN_TEST(test_full_transfer_walk_matches_expected_chunk_count);
    UNITY_END();
    return 0;
}
