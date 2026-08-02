#include "i2c.h"

#include "gpio.h"
#include "system_stm32f4xx.h"
#include "uart.h"

#include <stdint.h>

#define RCC_BASE 0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40))              // add 0x40 offset for APB1ENR register
#define RCC_AHB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x30))              // add 0x40 offset for APB1ENR register
#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x08)) // add 0x08 offset for CFGR register


#define GPIOB_BASE 0x40020400UL
#define GPIOB      ((GPIO_Port *)GPIOB_BASE)

#define I2C1_BASE 0x40005400UL
#define I2C1      ((I2C_TypeDef *)I2C1_BASE)

#define TIMEOUT (0x5000UL)

void i2c_init(uint32_t speed_hz)
{
    RCC_AHB1ENR |= (1 << 1);

    uint8_t i2c1_pins[2] = {8, 9}; // PB8=SCL, PB9=SDA

    for (int i = 0; i < 2; i++)
    {
        GPIO_Config cfg = {
            .mode      = GPIO_MODE_ALTERNATE,
            .otype     = GPIO_OTYPE_OPEN_DRAIN,
            .speed     = GPIO_SPEED_HIGH,
            .pupd      = GPIO_PUPD_PULLUP,
            .alternate = 4 // AF4 = I2C1 on STM32F4
        };
        GPIO_Init(GPIOB, i2c1_pins[i], cfg);
    }
    RCC_APB1ENR |= (1 << 21); // I2C1EN

    uint32_t pclk1_hz  = APB1_GetClock();
    uint32_t pclk1_mhz = pclk1_hz / 1000000UL;

    I2C1->CR2 = (I2C1->CR2 & ~0x3F) | (pclk1_mhz & 0x3F); // FREQ field, bits 5:0

    int32_t ccr_value;
    uint32_t trise_value;
    if (speed_hz <= 100000UL)
    {

        // Standard mode
        ccr_value   = pclk1_hz / (2UL * speed_hz);
        trise_value = pclk1_mhz + 1;

        I2C1->CCR = ccr_value & 0xFFF;
        // F/S bit (bit 15) and DUTY bit (bit 14) stay 0 — standard mode
    }
    else
    {
        // Fast mode (assumes DUTY=0, 2:1 duty cycle)
        ccr_value   = pclk1_hz / (3UL * speed_hz);
        trise_value = (pclk1_mhz * 300UL) / 1000UL + 1;

        I2C1->CCR = (1 << 15) | (ccr_value & 0xFFF); // F/S=1 (fast mode), DUTY=0
    }

    I2C1->TRISE = trise_value & 0x3F;

    I2C1->CR1 |= (1 << 0); // PE = 1, enable peripheral last
}

static void SR1_Error(void)
{
    while (1)
    {
    }
}

static void i2c_write_no_stop(uint8_t addr, const uint8_t *data, uint16_t len)
{
    I2C1->CR1 |= (1 << 8); // Start

    uint32_t timeout = TIMEOUT;

    while (!(I2C1->SR1 & (1 << 0))) // SB
    {
        if (--timeout == 0)
        {
            SR1_Error();
        }
    }

    I2C1->DR = (addr << 1) | 0;
    timeout  = TIMEOUT;

    while (!(I2C1->SR1 & (1 << 1))) // ADDR
    {
        if (--timeout == 0)
        {
            SR1_Error();
        }
    }

    // clear ADDR
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    timeout = TIMEOUT;

    for (uint16_t i = 0; i < len; i++)
    {
        while (!(I2C1->SR1 & (1 << 7))) // TxE
        {
            if (--timeout == 0)
            {
                I2C1->CR1 |= (1 << 9); // Stop to release bus
                SR1_Error();
            }
        }

        I2C1->DR = data[i];
    }

    timeout = TIMEOUT;
    while (!(I2C1->SR1 & (1 << 2))) // BTF
    {
        if (--timeout == 0)
        {
            I2C1->CR1 |= (1 << 9); // Stop to release bus
            SR1_Error();
        }
    }
}

void i2c_write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    i2c_write_no_stop(addr, data, len);

    I2C1->CR1 |= (1 << 9);
}

void i2c_read(uint8_t addr, uint8_t *data, uint16_t len)
{
    I2C1->CR1 |= (1 << 10); // ACK = 1

    I2C1->CR1 |= (1 << 8);  // Start

    uint32_t timeout = TIMEOUT;

    while (!(I2C1->SR1 & (1 << 0))) // SB
    {
        if (--timeout == 0)
        {
            SR1_Error();
        }
    }

    timeout = TIMEOUT;

    I2C1->DR = (addr << 1) | 1;
    while (!(I2C1->SR1 & (1 << 1))) // ADDR
    {
        if (--timeout == 0)
        {
            SR1_Error();
        }
    }

    timeout = TIMEOUT;

    if (len == 1)
    {
        // Special case: for a single-byte read, NACK must be armed
        // and STOP must be requested BEFORE clearing ADDR
        I2C1->CR1 &= ~(1 << 10); // ACK = 0 NACK the only byte we'll receive
        (void)I2C1->SR1;
        (void)I2C1->SR2;         // clears ADDR
        I2C1->CR1 |= (1 << 9);   // STOP
        while (!(I2C1->SR1 & (1 << 6)))
        {
            if (--timeout == 0)
            {
                SR1_Error();
            }
        }

        data[0] = (uint8_t)I2C1->DR;
        return;
    }

    // Multi-byte case: clear ADDR normally
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    for (uint16_t i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            // Last byte: must NACK + STOP before this byte is actually read out,
            // so it has to happen while the second-to-last byte is still transferring.
            I2C1->CR1 &= ~(1 << 10); // ACK = 0
            I2C1->CR1 |= (1 << 9);   // STOP
        }

        while (!(I2C1->SR1 & (1 << 6))) // RxNE
        {
            if (--timeout == 0)
            {
                SR1_Error();
            }
        }
        data[i] = (uint8_t)I2C1->DR;
    }
}

void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    i2c_write(addr, buf, 2);
}


uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    i2c_write_no_stop(addr, &reg, 1);
    uint8_t data[1];
    i2c_read(addr, data, 1);

    return data[0];
}
