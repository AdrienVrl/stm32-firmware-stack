#include "update_protocol.h"

uint32_t update_decode_u32_le(const uint8_t b[4])
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

void update_parse_header(const uint8_t header[8], uint32_t *image_size, uint32_t *crc)
{
    *image_size = update_decode_u32_le(&header[0]);
    *crc        = update_decode_u32_le(&header[4]);
}

uint32_t update_next_chunk_len(uint32_t image_size, uint32_t offset)
{
    uint32_t remaining;

    if (offset >= image_size)
    {
        return 0u;
    }
    remaining = image_size - offset;
    return (remaining < UPDATE_CHUNK_SIZE) ? remaining : UPDATE_CHUNK_SIZE;
}

bool update_is_last_chunk(uint32_t image_size, uint32_t offset, uint32_t chunk_len)
{
    return (offset + chunk_len) >= image_size;
}
