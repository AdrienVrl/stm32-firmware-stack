#include "spi.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);
    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};

    spi_init(cfg);

    uint8_t received = spi_transfer_byte(0xA5);
    printf("sent 0xA5, got 0x%02X\r\n", received);
}
