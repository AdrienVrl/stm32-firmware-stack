#include "pwm.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);
    pwm_init(1000);
    while (1)
    {
        for (uint8_t d = 0; d <= 100; d++)
        {
            pwm_set_duty(d);
            for (volatile uint32_t i = 0; i < d; i++)
            {
            }
        }
        for (uint8_t d = 100; d > 0; d--)
        {
            pwm_set_duty(d - 1);

            for (volatile uint32_t i = 0; i < d; i++)
            {
            }
        }
    }
}
