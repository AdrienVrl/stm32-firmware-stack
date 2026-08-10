#include "exti.h"
#include "gpio.h"
#include "i2c.h"
#include "pwm.h"
#include "spi.h"
#include "uart.h"

#include <stdint.h>
#include <string.h>

uint32_t SystemCoreClock     = 16000000UL;
GPIO_Port mock_gpioa         = {0, 0, 0, 0, 0, 0, 0, 0, {0, 0}};
USART_Port mock_usart2       = {0xC0, 0, 0, 0, 0, 0, 0};
GPIO_Port empty_mock_gpio    = {0, 0, 0, 0, 0, 0, 0, 0, {0, 0}};
USART_Port empty_mock_usart2 = {0xC0, 0, 0, 0, 0, 0, 0};
SPI_TypeDef mock_spi         = {0, 0, 0, 0, 0, 0, 0, 0, 0};
SPI_TypeDef empty_mock_spi   = {0, 0, 0, 0, 0, 0, 0, 0, 0};
I2C_TypeDef mock_i2c         = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
I2C_TypeDef empty_mock_i2c   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
EXTI_TypeDef mock_exti       = {0, 0, 0, 0, 0, 0};
EXTI_TypeDef empty_mock_exti = {0, 0, 0, 0, 0, 0};
TIM_TypeDef mock_tim         = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
TIM_TypeDef empty_mock_tim   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint32_t mock_rcc_ahb1enr   = 0;
uint32_t mock_rcc_apb1enr   = 0;
uint32_t mock_rcc_apb2enr   = 0;
uint32_t mock_rcc_cfgr      = 0;
uint32_t mock_nvic_iser0    = 0;
uint32_t mock_nvic_iser1    = 0;
uint32_t mock_syscfg_exticr = 0;
uint8_t mock_nvic_ipr[64]   = {0};

void mock_registers_reset(void)
{
    mock_gpioa  = empty_mock_gpio;
    mock_usart2 = empty_mock_usart2;
    mock_spi    = empty_mock_spi;
    mock_i2c    = empty_mock_i2c;
    mock_exti   = empty_mock_exti;
    mock_tim    = empty_mock_tim;

    mock_rcc_ahb1enr   = 0;
    mock_rcc_apb1enr   = 0;
    mock_rcc_apb2enr   = 0;
    mock_rcc_cfgr      = 0;
    mock_nvic_iser0    = 0;
    mock_nvic_iser1    = 0;
    mock_syscfg_exticr = 0;
    memset(mock_nvic_ipr, 0, sizeof(mock_nvic_ipr));
}
