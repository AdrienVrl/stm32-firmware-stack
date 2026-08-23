#ifndef BOOTLOADER_UPDATE_H
#define BOOTLOADER_UPDATE_H
#include <stdbool.h>
#include <stdint.h>

#define UPDATE_MAGIC_LEN   4u
#define UPDATE_MAGIC_BYTES {'U', 'P', 'D', 'T'}

#define UPDATE_ACK        0x06u
#define UPDATE_NACK       0x15u
#define UPDATE_CHUNK_SIZE 256u
bool update_try_enter(void);
void update_run_session(void);

#endif
