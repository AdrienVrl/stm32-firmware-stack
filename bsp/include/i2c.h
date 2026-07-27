#ifndef I2C_H
#define I2C_H
#include <stdint.h>

typedef struct
{
    volatile uint32_t CR1;   // 0x00
    volatile uint32_t CR2;   // 0x04
    volatile uint32_t OAR1;  // 0x08
    volatile uint32_t OAR2;  // 0x0C
    volatile uint32_t DR;    // 0x10
    volatile uint32_t SR1;   // 0x14
    volatile uint32_t SR2;   // 0x18
    volatile uint32_t CCR;   // 0x1C
    volatile uint32_t TRISE; // 0x20
    volatile uint32_t FLTR;  // 0x24
} I2C_TypeDef;

void i2c_init(uint32_t speed_hz);
void i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
void i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
void i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);
uint8_t i2c_read_reg(uint8_t addr, uint8_t reg);
#endif
