#ifndef BOOTLOADER_UPDATE_PROTOCOL_H
#define BOOTLOADER_UPDATE_PROTOCOL_H
#include <stdbool.h>
#include <stdint.h>
#define UPDATE_CHUNK_SIZE 256u
uint32_t update_decode_u32_le(const uint8_t b[4]);
void update_parse_header(const uint8_t header[8], uint32_t *image_size, uint32_t *crc);
uint32_t update_next_chunk_len(uint32_t image_size, uint32_t offset);
bool update_is_last_chunk(uint32_t image_size, uint32_t offset, uint32_t chunk_len);
#endif
