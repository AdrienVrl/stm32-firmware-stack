#include "uart.h"

#include <stdint.h>
#include <syscall.h>

int _write(int fd, char *buf, int len)
{
    if (fd != 1 && fd != 2)
    {
        return -1;
    }
    for (int i = 0; i < len; i++)
    {
        uart_write_byte((uint8_t)buf[i]);
    }
    return len;
}
