#ifndef BSP_CRC32_H
#define BSP_CRC32_H
#include <stddef.h>
#include <stdint.h>

uint32_t crc32_init(void);
uint32_t crc32_update(uint32_t state, const uint8_t *data, size_t len);
uint32_t crc32_finalize(uint32_t state);

#endif
