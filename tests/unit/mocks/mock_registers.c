#include "gpio.h"
#include "uart.h"

#include <stdint.h>

uint32_t SystemCoreClock     = 16000000UL;
GPIO_Port mock_gpioa         = {0, 0, 0, 0, 0, 0, 0, 0, {0, 0}};
USART_Port mock_usart2       = {0xC0, 0, 0, 0, 0, 0, 0};
GPIO_Port empty_mock_gpio    = {0, 0, 0, 0, 0, 0, 0, 0, {0, 0}};
USART_Port empty_mock_usart2 = {0xC0, 0, 0, 0, 0, 0, 0};

uint32_t mock_rcc_ahb1enr = 0;
uint32_t mock_rcc_apb1enr = 0;
uint32_t mock_rcc_cfgr    = 0;
uint32_t mock_nvic_iser1  = 0;

void mock_registers_reset(void)
{
    mock_gpioa  = empty_mock_gpio;
    mock_usart2 = empty_mock_usart2;

    mock_rcc_ahb1enr = 0;
    mock_rcc_apb1enr = 0;
    mock_rcc_cfgr    = 0;
    mock_nvic_iser1  = 0;
}
