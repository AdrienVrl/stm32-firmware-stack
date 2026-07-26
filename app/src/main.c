#include "i2c.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);
    i2c_init(100000);

    uint8_t who_am_i;
    who_am_i = i2c_read_reg(0x68, 0x75);
    printf("WHO_AM_I = 0x%02X\r\n", who_am_i);
}
