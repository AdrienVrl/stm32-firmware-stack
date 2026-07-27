#include "input_capture.h"
#include "pwm.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uart_init(115200);
    uint32_t test_freqs[] = {100, 500, 1000, 5000, 10000, 20000};
    int n                 = sizeof(test_freqs) / sizeof(test_freqs[0]);
    input_capture_init();

    for (int i = 0; i < n; i++)
    {
        // Re-init PWM at the new frequency (pwm_init recomputes PSC/ARR)
        pwm_init(test_freqs[i]);
        pwm_set_duty(50);

        if (!input_capture_wait_fresh())
        {
            printf("Set: %6lu Hz | TIMEOUT waiting for captures\r\n", test_freqs[i]);
            continue;
        }
        uint32_t measured = input_capture_get_frequency_hz();

        int32_t error   = (int32_t)measured - (int32_t)test_freqs[i];
        float error_pct = (error * 100.0f) / (float)test_freqs[i];

        printf("Set: %6lu Hz | Measured: %6lu Hz | Error: %5ld Hz (%.2f%%)\r\n", test_freqs[i],
               measured, error, error_pct);
    }
}
