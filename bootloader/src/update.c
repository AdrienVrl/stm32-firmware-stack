#include "update.h"

#include "crc32.h"
#include "flash.h"
#include "uart.h"
#include "update_protocol.h"

#include <stddef.h>

#define SCB_AIRCR         (*(volatile uint32_t *)0xE000ED0Cu)
#define AIRCR_VECTKEY     0x05FA0000u
#define AIRCR_SYSRESETREQ (1u << 2)

static void system_reset(void)
{
    uart_wait_tx_complete();
    __asm volatile("dsb");
    SCB_AIRCR = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
    __asm volatile("isb");
    for (;;)
    {
    }
}

#define UPDATE_HANDSHAKE_TIMEOUT_ITERS 8000000u

static uint8_t update_read_byte_blocking(void)
{
    uint8_t byte;
    while (!uart_read_byte(&byte))
    {
        /* spin */
    }
    return byte;
}

static void update_read_bytes_blocking(uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
    {
        buf[i] = update_read_byte_blocking();
    }
}

bool update_try_enter(void)
{
    static const uint8_t magic[UPDATE_MAGIC_LEN] = UPDATE_MAGIC_BYTES;
    uint32_t timeout                             = UPDATE_HANDSHAKE_TIMEOUT_ITERS;
    size_t matched                               = 0;

    while (timeout > 0u)
    {
        uint8_t byte;
        if (uart_read_byte(&byte))
        {
            if (byte == magic[matched])
            {
                matched++;
                if (matched == UPDATE_MAGIC_LEN)
                {
                    uart_write_byte(UPDATE_ACK);
                    return true;
                }
            }
            else
            {
                matched = (byte == magic[0]) ? 1u : 0u;
            }
        }
        else
        {
            timeout--;
        }
    }
    return false;
}

void update_run_session(void)
{
    static uint8_t chunk_buf[UPDATE_CHUNK_SIZE];
    uint8_t header[8];
    uint32_t image_size;
    uint32_t expected_crc;
    uint32_t running_crc;
    uint32_t offset;
    flash_status_t status;

    update_read_bytes_blocking(header, sizeof(header));
    update_parse_header(header, &image_size, &expected_crc);

    if (image_size == 0u || image_size > FLASH_APP_REGION_SIZE)
    {
        uart_write_byte(UPDATE_NACK);
        system_reset();
    }

    flash_unlock();

    status = flash_erase_app_region();
    if (status != FLASH_OK)
    {
        flash_lock();
        uart_write_byte(UPDATE_NACK);
        system_reset();
    }
    uart_write_byte(UPDATE_ACK);

    running_crc = crc32_init();
    offset      = 0u;

    while (offset < image_size)
    {
        uint32_t this_chunk = update_next_chunk_len(image_size, offset);
        bool is_last        = update_is_last_chunk(image_size, offset, this_chunk);

        update_read_bytes_blocking(chunk_buf, this_chunk);

        status      = flash_program(FLASH_APP_BASE_ADDR + offset, chunk_buf, this_chunk);
        running_crc = crc32_update(running_crc, chunk_buf, this_chunk);

        if (status != FLASH_OK)
        {
            flash_lock();
            uart_write_byte(UPDATE_NACK);
            system_reset();
        }

        offset += this_chunk;

        if (is_last)
        {
            uint32_t final_crc = crc32_finalize(running_crc);
            flash_lock();
            uart_write_byte((final_crc == expected_crc) ? UPDATE_ACK : UPDATE_NACK);
        }
        else
        {
            uart_write_byte(UPDATE_ACK);
        }
    }

    system_reset();
}
