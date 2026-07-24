#ifndef SPI_H
#define SPI_H

#include <stdint.h>

typedef enum
{
    SPI_PRESCALER_2   = 0x0,
    SPI_PRESCALER_4   = 0x1,
    SPI_PRESCALER_8   = 0x2,
    SPI_PRESCALER_16  = 0x3,
    SPI_PRESCALER_32  = 0x4,
    SPI_PRESCALER_64  = 0x5,
    SPI_PRESCALER_128 = 0x6,
    SPI_PRESCALER_256 = 0x7
} SPI_Prescaler;

typedef enum
{
    SPI_MODE0 = 0x0, // CPOL=0, CPHA=0 — clock idle low,  sample on rising edge  (most common)
    SPI_MODE1 = 0x1, // CPOL=0, CPHA=1 — clock idle low,  sample on falling edge
    SPI_MODE2 = 0x2, // CPOL=1, CPHA=0 — clock idle high, sample on falling edge
    SPI_MODE3 = 0x3  // CPOL=1, CPHA=1 — clock idle high, sample on rising edge
} SPI_Mode;

typedef struct
{
    SPI_Prescaler prescaler;
    SPI_Mode mode;
} SPI_Config;

typedef struct
{
    volatile uint32_t CR1;     // 0x00 Control register 1
    volatile uint32_t CR2;     // 0x04 Control register 2
    volatile uint32_t SR;      // 0x08 Status register
    volatile uint32_t DR;      // 0x0C Data register
    volatile uint32_t CRCPR;   // 0x10 CRC polynomial register
    volatile uint32_t RXCRCR;  // 0x14 RX CRC register
    volatile uint32_t TXCRCR;  // 0x18 TX CRC register
    volatile uint32_t I2SCFGR; // 0x1C I2S configuration register
    volatile uint32_t I2SPR;   // 0x20 I2S prescaler register
} SPI_TypeDef;

void spi_init(SPI_Config cfg);

uint8_t spi_transfer_byte(uint8_t data);

void spi_write(const uint8_t *data, uint16_t len);

void spi_read(uint8_t *data, uint16_t len);

#endif
