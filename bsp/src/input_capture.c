#include "input_capture.h"

#include "gpio.h"
#include "uart.h"

#include <pwm.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef UNIT_TEST
#define RCC_BASE 0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40))                // add 0x40 offset for APB1ENR register

#define RCC_CFGR   (*(volatile uint32_t *)(RCC_BASE + 0x08)) // add 0x08 offset for CFGR register
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100UL)


#define TIM3_BASE 0x40000400UL
#define TIM3      ((TIM_TypeDef *)TIM3_BASE)

#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)
#else
#include "mock_registers.h"
#define RCC_APB1ENR mock_rcc_apb1enr
#define RCC_CFGR    mock_rcc_cfgr
#define GPIOA_BASE  ((uintptr_t) & mock_gpioa)
#define GPIOA       ((GPIO_Port *)GPIOA_BASE)
#define TIM3_BASE   ((uintptr_t) & mock_tim)
#define TIM3        ((TIM_TypeDef *)TIM3_BASE)
#define NVIC_ISER0  mock_nvic_iser0
#endif

void input_capture_init(void)
{
    RCC_APB1ENR |= (1 << 1);
    volatile uint32_t dummy;
    dummy = RCC_APB1ENR;
    (void)dummy;

    GPIO_Config cfg = {
        .mode = GPIO_MODE_ALTERNATE,
        .otype =
            GPIO_OTYPE_PUSH_PULL, // driving an LED, push-pull is correct (not open-drain like I2C)
        .speed     = GPIO_SPEED_HIGH,
        .pupd      = GPIO_PUPD_NONE, // actively driven, no pull needed
        .alternate = 2               // AF2 = TIM3 on STM32F4
    };
    GPIO_Init(GPIOA, 6, cfg);


    TIM3->CCMR1 = (TIM3->CCMR1 & ~(0x3 << 0)) | (0x1 << 0); // CC1S = 01:
    TIM3->CCMR1 &= ~(0x3 << 2);                             // ICPSC = 00
    TIM3->CCMR1 &= ~(0xF << 4);                             // ICF = 0000
    TIM3->CCER &= ~(1 << 1);                                // CC1P = 0
    TIM3->CCER &= ~(1 << 3);                                // CC1NP = 0
    TIM3->CCER |= (1 << 0);                                 // CC1E
    TIM3->DIER |= (1 << 1);                                 // CC1IE
    TIM3->PSC = 0;
    TIM3->ARR = 0xFFFF;
    TIM3->EGR |= (1 << 0);   // UG
    TIM3->CR1 |= (1 << 0);   // CEN

    TIM3->DIER |= (1 << 0);  // Update Interrup Enable
    NVIC_ISER0 |= (1 << 29); // Unable IRQ
}

static volatile uint16_t prev_capture      = 0;
static volatile uint32_t overflow_count    = 0;
static volatile uint32_t last_period_ticks = 0;
static volatile bool capture_ready         = false;

uint32_t input_capture_get_frequency_hz(void)
{

    if (!capture_ready || last_period_ticks == 0)
    {
        return 0;
    }

    uint32_t pclk1     = APB1_GetClock();
    uint32_t ppre1     = (RCC_CFGR >> 10) & 0x7;
    uint32_t timer_clk = ((ppre1 & 0x4) == 0) ? pclk1 : (pclk1 * 2);
    uint32_t freq_hz   = timer_clk / last_period_ticks;

    return freq_hz;
}

void TIM3_IRQHandler(void)
{
    // Track overflow
    if (TIM3->SR & (1 << 0))
    {                          // UIF
        TIM3->SR &= ~(1 << 0); // clear UIF
        overflow_count++;
    }

    // Capture input
    if (TIM3->SR & (1 << 1))
    {                          // CC1IF
        TIM3->SR &= ~(1 << 1); // clear CC1IF

        uint16_t current_capture = (uint16_t)TIM3->CCR1;

        // Wraparound-safe difference within the current overflow window
        uint16_t tick_diff_in_window = (uint16_t)(current_capture - prev_capture);

        // Total period = full overflow cycles since last capture, plus the remainder
        if (current_capture >= prev_capture)
        {
            last_period_ticks = (overflow_count * 0x10000UL) + tick_diff_in_window;
        }
        else
        {
            last_period_ticks = ((overflow_count - 1) * 0x10000UL) + tick_diff_in_window;
        }

        prev_capture   = current_capture;
        overflow_count = 0; // reset — we've now accounted for it in this period
        capture_ready  = true;
    }
}
