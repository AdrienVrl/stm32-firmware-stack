#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

uint32_t APB1_GetClock(void);

void uart_init(uint32_t baud_rate);

void uart_write_byte(uint8_t byte);

void uart_write_str(const char *str);

void uart_write_u32(uint32_t value);

bool uart_read_byte(uint8_t *out);

#endif
