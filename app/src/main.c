#include "gpio.h"

#include <stdint.h>

#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)
#define GPIOC      ((GPIO_Port *)(GPIOA_BASE + 0x800UL))

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

    GPIO_Config b1_cfg = {.mode = GPIO_MODE_INPUT,
                          .otype =
                              GPIO_OTYPE_PUSH_PULL,    // don't care in input mode, left at default
                          .speed     = GPIO_SPEED_LOW, // don't care in input mode
                          .pupd      = GPIO_PUPD_PULLUP, // external pull already present on B1
                          .alternate = 0};
    GPIO_Init(GPIOC, 13, b1_cfg);

    while (1)
    {
        GPIO_PinState b1_state = GPIO_ReadPin(GPIOC, 13);
        GPIO_WritePin(GPIOA, 5, b1_state);
    }
}
