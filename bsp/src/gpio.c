#include <gpio.h>
#include <stdint.h>
#define RCC_BASE 0x40023800
#define RCC_AHB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x30)) // add 0x30 offset for AHB1ENR register
#define GPIOA_BASE 0x40020000UL

void GPIO_Init(GPIO_Port *port, uint8_t pin, GPIO_Config config)
{
    volatile uint32_t dummy;
    uint32_t offset = ((uint32_t)port - GPIOA_BASE) /
                      0x400UL; // divides by port size (0x400UL) to get port number
    RCC_AHB1ENR |= (1 << offset);
    dummy = RCC_AHB1ENR;
    (void)dummy;

    port->MODER &= ~(0x3 << (pin * 2));
    port->MODER |= (config.mode << (pin * 2));

    port->OTYPER &= ~(0x1 << pin);
    port->OTYPER |= (config.otype << pin);

    port->OSPEEDR &= ~(0x3 << (pin * 2));
    port->OSPEEDR |= (config.speed << (pin * 2));

    port->PUPDR &= ~(0x3 << (pin * 2));
    port->PUPDR |= (config.pupd << (pin * 2));

    if (config.mode == GPIO_MODE_ALTERNATE)
    {
        uint8_t afr_index = pin / 8; // whether the pin sits in the low or high register
        uint8_t afr_shift =
            (pin % 8) * 4;           // position of the pin within the register (4 bits per pin)
        port->AFR[afr_index] &= ~(0xF << afr_shift);
        port->AFR[afr_index] |= (config.alternate << afr_shift);
    }
}

void GPIO_WritePin(GPIO_Port *port, uint8_t pin, uint8_t value)
{
    if (value)
        port->ODR |= (1 << pin);
    else
        port->ODR &= ~(1 << pin);
}

void GPIO_TogglePin(GPIO_Port *port, uint8_t pin)
{
    if (port->ODR & (1 << pin))
        port->ODR &= ~(1 << pin); // currently high -> set low
    else
        port->ODR |= (1 << pin);  // currently low -> set high
}

GPIO_PinState GPIO_ReadPin(GPIO_Port *port, uint8_t pin)
{
    if (port->IDR & (1 << pin))
        return GPIO_PIN_SET;
    else
        return GPIO_PIN_RESET;
}
