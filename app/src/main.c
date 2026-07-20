#include "uart.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);
    printf("hello %d\r\n", 42);
}
