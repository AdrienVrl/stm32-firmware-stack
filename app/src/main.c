#include "spi.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>

volatile uint32_t background_counter = 0;

int main(void)
{
    uart_init(115200);
    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};

    spi_init(cfg);
    spi_dma_init();

    uint8_t tx_buf[16];
    uint8_t rx_buf[16];
    for (int i = 0; i < 16; i++)
        tx_buf[i] = i;
    spi_transfer_dma(tx_buf, rx_buf, 16);
    while (spi_dma_busy())
    {
        background_counter++;
    }

    printf("Background counter during TX: %lu\r\n", background_counter);

    bool match = true;
    for (int i = 0; i < 16; i++)
    {
        if (rx_buf[i] != tx_buf[i])
        {
            match = false;
            printf("Mismatch at %d: sent 0x%02X, got 0x%02X\r\n", i, tx_buf[i], rx_buf[i]);
        }
    }
    printf(match ? "Loopback OK, 32 bytes matched\r\n" : "Loopback FAILED\r\n");

    printf("match: %d\r\n", match);
}
