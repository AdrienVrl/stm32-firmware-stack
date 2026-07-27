#include "uart.h"
#include "pwm.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);

    pwm_init(1000);       // 1 kHz PWM

    pwm_set_duty(0);

    for (uint8_t d = 0; d <= 100; d++) {
        pwm_set_duty(d);
        // some delay between steps
    }
}
