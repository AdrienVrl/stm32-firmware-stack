#include "uart.h"

#include <stdint.h>
#include <stdio.h>

#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)
#define GPIOC      ((GPIO_Port *)(GPIOA_BASE + 0x800UL))

int main(void)
{
    uart_init(115200);
    printf("hello %d\r\n", 42);
}
