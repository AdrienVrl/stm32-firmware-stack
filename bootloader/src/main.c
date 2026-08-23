#include "common.h"
#include "gpio.h"
#include "uart.h"
#include "update.h"

#include <stdint.h>
#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)

int main(void)
{
    GPIO_Config ld2_cfg = {.mode      = GPIO_MODE_OUTPUT,
                           .otype     = GPIO_OTYPE_PUSH_PULL,
                           .speed     = GPIO_SPEED_LOW,
                           .pupd      = GPIO_PUPD_NONE,
                           .alternate = 0};
    GPIO_Init(GPIOA, 5, ld2_cfg);
    uart_init(115200);

    if (update_try_enter())
    {
        update_run_session();
    }

    uint32_t stack_pointer = *(uint32_t *)0x08008000U;

    if ((stack_pointer < 0x20000000U) || (stack_pointer > 0x20020000U))
    {

        while (1)
        {
            GPIO_TogglePin(GPIOA, 5);
            for (volatile int i = 0; i < 1000000; i++)
            {
            }
        }
    }

    uint32_t reset_handler = *(uint32_t *)0x08008004U;

    __disable_irq();
    SCB_VTOR = 0x08008000U;
    __set_MSP(stack_pointer);
    ((void (*)(void))reset_handler)();
}
