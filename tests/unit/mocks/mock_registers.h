#ifndef MOCK_REGISTERS_H
#define MOCK_REGISTERS_H
#include "exti.h"
#include "gpio.h"
#include "i2c.h"
#include "pwm.h"
#include "spi.h"
#include "uart.h"

#include <stdint.h>

extern GPIO_Port mock_gpioa;
extern USART_Port mock_usart2;
extern SPI_TypeDef mock_spi;
extern I2C_TypeDef mock_i2c;
extern EXTI_TypeDef mock_exti;
extern TIM_TypeDef mock_tim;

extern uint32_t mock_rcc_ahb1enr;
extern uint32_t mock_rcc_apb1enr;
extern uint32_t mock_rcc_apb2enr;
extern uint32_t mock_rcc_cfgr;
extern uint32_t mock_syscfg_exticr;
extern uint32_t mock_nvic_iser0;
extern uint32_t mock_nvic_iser1;
extern uint8_t mock_nvic_ipr[64];

void mock_registers_reset(void);

#endif
