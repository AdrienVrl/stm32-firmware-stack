#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

uint32_t APB1_GetClock(void);

typedef struct
{
    volatile uint32_t SR;   // 0x00 Status register
    volatile uint32_t DR;   // 0x04 Data register
    volatile uint32_t BRR;  // 0x08 Baud rate register
    volatile uint32_t CR1;  // 0x0C Control register 1
    volatile uint32_t CR2;  // 0x10 Control register 2
    volatile uint32_t CR3;  // 0x14 Control register 3
    volatile uint32_t GTPR; // 0x18 Guard time and prescaler
} USART_Port;

void uart_init(uint32_t baud_rate);

void uart_write_byte(uint8_t byte);

void uart_write_str(const char *str);

void uart_wait_tx_complete(void);

void uart_write_u32(uint32_t value);

bool uart_read_byte(uint8_t *out);

#endif
