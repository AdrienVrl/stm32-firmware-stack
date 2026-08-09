#ifndef MOCK_REGISTERS_H
#define MOCK_REGISTERS_H
#include "gpio.h"
#include "uart.h"

#include <stdint.h>

extern GPIO_Port mock_gpioa;
extern USART_Port mock_usart2;

extern uint32_t mock_rcc_ahb1enr;
extern uint32_t mock_rcc_apb1enr;
extern uint32_t mock_rcc_cfgr;
extern uint32_t mock_nvic_iser1;

void mock_registers_reset(void);

#endif
