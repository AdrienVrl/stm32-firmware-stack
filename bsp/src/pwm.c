#include "pwm.h"

#include "gpio.h"
#include "uart.h"

#include <stdint.h>

#define RCC_BASE 0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40))              // add 0x40 offset for APB1ENR register
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x08)) // add 0x08 offset for CFGR register

#define TIM2_BASE 0x40000000UL
#define TIM2      ((TIM_TypeDef *)TIM2_BASE)

#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)


void pwm_init(uint32_t freq_hz)
{
    RCC_APB1ENR |= (1 << 0);
    uint32_t pclk1     = APB1_GetClock();
    uint32_t ppre1     = (RCC_CFGR >> 10) & 0x7;
    uint32_t timer_clk = (ppre1 == 0) ? pclk1 : (pclk1 * 2);

    uint32_t total_ticks = timer_clk / freq_hz; // = (PSC+1) * (ARR+1)

    uint32_t psc = 0;
    uint32_t arr = total_ticks - 1;

    // If total_ticks doesn't fit in ARR's 16-bit range, increase PSC until it does
    while (arr > 0xFFFF)
    {
        psc++;
        arr = (total_ticks / (psc + 1)) - 1;
    }

    TIM2->PSC = psc;
    TIM2->ARR = arr;

    TIM2->CCMR1 = (TIM2->CCMR1 & ~(0x7 << 4)) | (0x6 << 4);
    TIM2->CCMR1 |= (1 << 3);
    TIM2->CCER |= (1 << 0);
    TIM2->CR1 |= (1 << 7);
    TIM2->EGR |= (1 << 0);
    TIM2->CR1 |= (1 << 0);
    GPIO_Config cfg = {
        .mode = GPIO_MODE_ALTERNATE,
        .otype =
            GPIO_OTYPE_PUSH_PULL, // driving an LED, push-pull is correct (not open-drain like I2C)
        .speed     = GPIO_SPEED_HIGH,
        .pupd      = GPIO_PUPD_NONE, // actively driven, no pull needed
        .alternate = 1               // AF1 = TIM2 on STM32F4
    };
    GPIO_Init(GPIOA, 5, cfg);
}

void pwm_set_duty(uint8_t duty_percent)
{
    if (duty_percent > 100)
    {
        duty_percent = 100;
    }

    uint32_t arr  = TIM2->ARR;
    uint32_t ccr1 = ((uint32_t)duty_percent * arr) / 100;

    TIM2->CCR1 = ccr1;
}
