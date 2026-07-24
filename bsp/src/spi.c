#include "spi.h"

#include "gpio.h"

#include <stdint.h>

#define SPI_DUMMY_BYTE 0xFF // filler value to clock out while reading
#define RCC_BASE       0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40)) // add 0x40 offset for APB1ENR register

#define GPIOB_BASE 0x40020400UL
#define GPIOB      ((GPIO_Port *)GPIOB_BASE)

#define SPI2_BASE 0x40003800UL
#define SPI2      ((SPI_TypeDef *)SPI2_BASE)

#define TIMEOUT (0x5000UL)


static void RXNE_Error(void)
{
    while (1)
    {
    }
}

static void TXE_Error(void)
{
    while (1)
    {
    }
}

static inline void spi_cs_select(void)
{
    GPIO_WritePin(GPIOB, 12, 0); // pull CS low — assert
}

static inline void spi_cs_deselect(void)
{
    GPIO_WritePin(GPIOB, 12, 1); // release CS high — deassert
}

void spi_init(SPI_Config cfg)
{
    // PB12: software-controlled CS, plain GPIO output — NOT alternate function
    GPIO_Config cs_cfg = {.mode  = GPIO_MODE_OUTPUT,
                          .otype = GPIO_OTYPE_PUSH_PULL,
                          .speed = GPIO_SPEED_VERY_HIGH,
                          .pupd  = GPIO_PUPD_NONE};
    GPIO_Init(GPIOB, 12, cs_cfg);
    uint8_t spi2_pins[3] = {13, 14, 15}; // SCK, MISO, MOSI

    for (int i = 0; i < 3; i++)
    {
        GPIO_Config gpio_cfg = {
            .mode      = GPIO_MODE_ALTERNATE,
            .otype     = GPIO_OTYPE_PUSH_PULL,
            .speed     = GPIO_SPEED_VERY_HIGH, // SPI clock lines benefit from fast slew rate
            .pupd      = GPIO_PUPD_NONE,
            .alternate = 5                     // AF5 = SPI1/SPI2 on STM32F4
        };
        GPIO_Init(GPIOB, spi2_pins[i], gpio_cfg);
    }

    RCC_APB1ENR |= (1 << 14);
    volatile uint32_t dummy;
    dummy = RCC_APB1ENR;
    (void)dummy;

    uint32_t cr1 = 0;

    cr1 |= (cfg.mode & 0x3);     // bits 0-1: CPHA, CPOL
    cr1 |= (cfg.prescaler << 3); // bits 3-5: BR[2:0]
    cr1 |= (1 << 2);             // bit 2: MSTR — master mode
    cr1 |= (1 << 8);             // bit 8: SSI — internal slave select high
    cr1 |= (1 << 9);             // bit 9: SSM — software slave management

    SPI2->CR1 = cr1;

    SPI2->CR1 |= (1 << 6); // bit 6: SPE — enable peripheral, set last
}

uint8_t spi_transfer_byte(uint8_t data)
{
    uint32_t timeout = TIMEOUT;

    while (!(SPI2->SR & (1 << 1)))
    {

        if (--timeout == 0UL)
        {
            TXE_Error();
        }
    }
    SPI2->DR = data;

    timeout = TIMEOUT;

    while (!(SPI2->SR & (1 << 0)))
    {

        if (--timeout == 0UL)
        {
            RXNE_Error();
        }
    }
    return (uint8_t)SPI2->DR;
}

void spi_write(const uint8_t *data, uint16_t len)
{
    spi_cs_select();
    for (uint16_t i = 0; i < len; i++)
    {
        spi_transfer_byte(data[i]);
    }
    spi_cs_deselect();
}

void spi_read(uint8_t *data, uint16_t len)
{
    spi_cs_select();
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = spi_transfer_byte(SPI_DUMMY_BYTE);
    }
    spi_cs_deselect();
}
