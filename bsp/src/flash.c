#include "flash.h"

#define FLASH_R_BASE 0x40023C00u

#define FLASH_ACR     (*(volatile uint32_t *)(FLASH_R_BASE + 0x00u))
#define FLASH_KEYR    (*(volatile uint32_t *)(FLASH_R_BASE + 0x04u))
#define FLASH_OPTKEYR (*(volatile uint32_t *)(FLASH_R_BASE + 0x08u))
#define FLASH_SR      (*(volatile uint32_t *)(FLASH_R_BASE + 0x0Cu))
#define FLASH_CR      (*(volatile uint32_t *)(FLASH_R_BASE + 0x10u))
#define FLASH_OPTCR   (*(volatile uint32_t *)(FLASH_R_BASE + 0x14u))

#define FLASH_KEY1 0x45670123u
#define FLASH_KEY2 0xCDEF89ABu

#define FLASH_SR_EOP    (1u << 0)
#define FLASH_SR_OPERR  (1u << 1)
#define FLASH_SR_WRPERR (1u << 4)
#define FLASH_SR_PGAERR (1u << 5)
#define FLASH_SR_PGPERR (1u << 6)
#define FLASH_SR_PGSERR (1u << 7)
#define FLASH_SR_BSY    (1u << 16)
#define FLASH_SR_ERR_MASK                                                                          \
    (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)

#define FLASH_CR_PG        (1u << 0)
#define FLASH_CR_SER       (1u << 1)
#define FLASH_CR_MER       (1u << 2)
#define FLASH_CR_SNB_POS   3u
#define FLASH_CR_SNB_MSK   (0x1Fu << FLASH_CR_SNB_POS)
#define FLASH_CR_PSIZE_POS 8u
#define FLASH_CR_PSIZE_MSK (0x3u << FLASH_CR_PSIZE_POS)
#define FLASH_CR_PSIZE_X32 (0x2u << FLASH_CR_PSIZE_POS)
#define FLASH_CR_STRT      (1u << 16)
#define FLASH_CR_LOCK      (1u << 31)

#define FLASH_BUSY_TIMEOUT_ITERS 20000000u

static flash_status_t flash_wait_busy(void)
{
    uint32_t timeout = FLASH_BUSY_TIMEOUT_ITERS;
    while ((FLASH_SR & FLASH_SR_BSY) != 0u)
    {
        if (--timeout == 0u)
        {
            return FLASH_ERR_BUSY_TIMEOUT;
        }
    }
    return FLASH_OK;
}

static flash_status_t flash_check_and_clear_errors(void)
{
    uint32_t sr           = FLASH_SR;
    flash_status_t status = FLASH_OK;

    if (sr & FLASH_SR_WRPERR)
        status = FLASH_ERR_WRPERR;
    else if (sr & FLASH_SR_PGAERR)
        status = FLASH_ERR_PGAERR;
    else if (sr & FLASH_SR_PGPERR)
        status = FLASH_ERR_PGPERR;
    else if (sr & FLASH_SR_PGSERR)
        status = FLASH_ERR_PGSERR;
    else if (sr & FLASH_SR_OPERR)
        status = FLASH_ERR_OPERR;

    FLASH_SR = sr & (FLASH_SR_ERR_MASK | FLASH_SR_EOP);

    return status;
}

void flash_unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK)
    {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

void flash_lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

flash_status_t flash_erase_sector(uint8_t sector)
{
    flash_status_t status;

    if (sector < FLASH_APP_START_SECTOR || sector > FLASH_APP_END_SECTOR)
    {
        return FLASH_ERR_BAD_SECTOR;
    }

    status = flash_wait_busy();
    if (status != FLASH_OK)
    {
        return status;
    }
    (void)flash_check_and_clear_errors();

    FLASH_CR =
        (FLASH_CR & ~FLASH_CR_SNB_MSK) | FLASH_CR_SER | ((uint32_t)sector << FLASH_CR_SNB_POS);
    FLASH_CR |= FLASH_CR_STRT;

    status = flash_wait_busy();

    FLASH_CR &= ~(FLASH_CR_SER | FLASH_CR_SNB_MSK);

    if (status != FLASH_OK)
    {
        return status;
    }
    return flash_check_and_clear_errors();
}

flash_status_t flash_erase_app_region(void)
{
    for (uint8_t s = FLASH_APP_START_SECTOR; s <= FLASH_APP_END_SECTOR; s++)
    {
        flash_status_t status = flash_erase_sector(s);
        if (status != FLASH_OK)
        {
            return status;
        }
    }
    return FLASH_OK;
}

flash_status_t flash_write_word(uint32_t addr, uint32_t data)
{
    flash_status_t status;

    if (addr < FLASH_APP_BASE_ADDR || addr >= (FLASH_APP_BASE_ADDR + FLASH_APP_REGION_SIZE) ||
        (addr & 0x3u) != 0u)
    {
        return FLASH_ERR_BAD_ADDR;
    }

    status = flash_wait_busy();
    if (status != FLASH_OK)
    {
        return status;
    }
    (void)flash_check_and_clear_errors();

    FLASH_CR = (FLASH_CR & ~FLASH_CR_PSIZE_MSK) | FLASH_CR_PSIZE_X32;
    FLASH_CR |= FLASH_CR_PG;

    *(volatile uint32_t *)addr = data;

    status = flash_wait_busy();

    FLASH_CR &= ~FLASH_CR_PG;

    if (status != FLASH_OK)
    {
        return status;
    }

    status = flash_check_and_clear_errors();
    if (status != FLASH_OK)
    {
        return status;
    }

    if (*(volatile uint32_t *)addr != data)
    {
        return FLASH_ERR_VERIFY;
    }

    return FLASH_OK;
}

flash_status_t flash_program(uint32_t addr, const uint8_t *data, size_t len)
{
    size_t i;

    if (addr < FLASH_APP_BASE_ADDR || (addr & 0x3u) != 0u)
    {
        return FLASH_ERR_BAD_ADDR;
    }
    if ((uint64_t)addr + (uint64_t)len > (uint64_t)(FLASH_APP_BASE_ADDR + FLASH_APP_REGION_SIZE))
    {
        return FLASH_ERR_BAD_ADDR;
    }

    i = 0;
    while (i < len)
    {
        uint32_t word    = 0xFFFFFFFFu;
        size_t remaining = len - i;
        size_t chunk     = (remaining < 4u) ? remaining : 4u;
        size_t b;

        for (b = 0; b < chunk; b++)
        {
            word &= ~(0xFFu << (8u * b));
            word |= ((uint32_t)data[i + b]) << (8u * b);
        }

        flash_status_t status = flash_write_word(addr + (uint32_t)i, word);
        if (status != FLASH_OK)
        {
            return status;
        }

        i += chunk;
    }

    return FLASH_OK;
}
