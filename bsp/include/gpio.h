#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t MODER;   // GPIO port mode register,        offset 0x00
    volatile uint32_t OTYPER;  // GPIO port output type register,  offset 0x04
    volatile uint32_t OSPEEDR; // GPIO port output speed register, offset 0x08
    volatile uint32_t PUPDR;   // GPIO port pull-up/down register, offset 0x0C
    volatile uint32_t IDR;     // GPIO port input data register,   offset 0x10
    volatile uint32_t ODR;     // GPIO port output data register,  offset 0x14
    volatile uint32_t BSRR;    // GPIO port bit set/reset register, offset 0x18
    volatile uint32_t LCKR;    // GPIO port configuration lock register, offset 0x1C
    volatile uint32_t AFR[2];  // GPIO alternate function low/high, offset 0x20-0x24
} GPIO_Port;

typedef enum
{
    GPIO_MODE_INPUT     = 0x0,
    GPIO_MODE_OUTPUT    = 0x1,
    GPIO_MODE_ALTERNATE = 0x2,
    GPIO_MODE_ANALOG    = 0x3
} GPIO_Mode;

typedef enum
{
    GPIO_OTYPE_PUSH_PULL  = 0x0,
    GPIO_OTYPE_OPEN_DRAIN = 0x1
} GPIO_OType;

typedef enum
{
    GPIO_SPEED_LOW       = 0x0,
    GPIO_SPEED_MEDIUM    = 0x1,
    GPIO_SPEED_HIGH      = 0x2,
    GPIO_SPEED_VERY_HIGH = 0x3
} GPIO_Speed;

typedef enum
{
    GPIO_PUPD_NONE     = 0x0,
    GPIO_PUPD_PULLUP   = 0x1,
    GPIO_PUPD_PULLDOWN = 0x2
} GPIO_PuPd;

typedef struct
{
    GPIO_Mode mode;
    GPIO_OType otype;
    GPIO_Speed speed;
    GPIO_PuPd pupd;
    uint8_t alternate; // AF function number (0-15), only used if mode == ALTERNATE
} GPIO_Config;

typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET   = 1
} GPIO_PinState;

void GPIO_Init(GPIO_Port *port, uint8_t pin, GPIO_Config config);

void GPIO_WritePin(GPIO_Port *port, uint8_t pin, uint8_t value);

GPIO_PinState GPIO_ReadPin(GPIO_Port *port, uint8_t pin);

#endif
