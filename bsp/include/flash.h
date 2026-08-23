#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERR_BUSY_TIMEOUT, /* FLASH_SR.BSY never cleared */
    FLASH_ERR_WRPERR,       /* write protection error */
    FLASH_ERR_PGAERR,       /* programming alignment error */
    FLASH_ERR_PGPERR,       /* programming parallelism error (PSIZE mismatch) */
    FLASH_ERR_PGSERR,       /* programming sequence error */
    FLASH_ERR_OPERR,        /* generic operation error */
    FLASH_ERR_BAD_SECTOR,   /* sector index out of range for this part */
    FLASH_ERR_BAD_ADDR,     /* address/length outside the app region, or misaligned */
    FLASH_ERR_VERIFY,       /* readback after program didn't match */
} flash_status_t;

#define FLASH_APP_START_SECTOR 2u
#define FLASH_APP_END_SECTOR   7u
#define FLASH_APP_BASE_ADDR    0x08008000u
#define FLASH_APP_REGION_SIZE  (480u * 1024u)

void flash_unlock(void);
void flash_lock(void);

flash_status_t flash_erase_sector(uint8_t sector);

flash_status_t flash_erase_app_region(void);

flash_status_t flash_write_word(uint32_t addr, uint32_t data);

flash_status_t flash_program(uint32_t addr, const uint8_t *data, size_t len);

#endif
