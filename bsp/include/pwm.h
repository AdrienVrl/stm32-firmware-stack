#ifndef PWM_H
#define PWM_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t CR1;   // 0x00
    volatile uint32_t CR2;   // 0x04
    volatile uint32_t SMCR;  // 0x08
    volatile uint32_t DIER;  // 0x0C
    volatile uint32_t SR;    // 0x10
    volatile uint32_t EGR;   // 0x14
    volatile uint32_t CCMR1; // 0x18
    volatile uint32_t CCMR2; // 0x1C
    volatile uint32_t CCER;  // 0x20
    volatile uint32_t CNT;   // 0x24
    volatile uint32_t PSC;   // 0x28
    volatile uint32_t ARR;   // 0x2C
    uint32_t RESERVED0;      // 0x30
    volatile uint32_t CCR1;  // 0x34
    volatile uint32_t CCR2;  // 0x38
    volatile uint32_t CCR3;  // 0x3C
    volatile uint32_t CCR4;  // 0x40
} TIM_TypeDef;

void pwm_init(uint32_t freq_hz);

void pwm_set_duty(uint8_t duty_percent);
#endif
