#include "gpio.h"

#include <stdint.h>
#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)

int main(void)
{
    GPIO_Config ld2_cfg = {
        .mode      = GPIO_MODE_OUTPUT,
        .otype     = GPIO_OTYPE_PUSH_PULL,
        .speed     = GPIO_SPEED_LOW,
        .pupd      = GPIO_PUPD_NONE,
        .alternate = 0 // unused, not in alternate mode
    };
    GPIO_Init(GPIOA, 5, ld2_cfg);

    while (1)
    {
        GPIO_TogglePin(GPIOA, 5);
        for (volatile int i = 0; i < 5000000; i++)
        {
        }
    }
}
