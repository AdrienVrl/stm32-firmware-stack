#include "gpio.h"
#include "system_stm32f4xx.h"

#include <stdbool.h>
#include <stdint.h>

#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)

#define USART2_BASE 0x40004400UL
#define USART2      ((USART_Port *)USART2_BASE)

#define RCC_BASE 0x40023800
#define RCC_APB1ENR                                                                                \
    (*(volatile uint32_t *)(RCC_BASE + 0x40))                // add 0x40 offset for APB1ENR register
#define RCC_CFGR   (*(volatile uint32_t *)(RCC_BASE + 0x08)) // add 0x08 offset for CFGR register
#define NVIC_ISER1 (*(volatile uint32_t *)0xE000E104UL)      // NVIC address

#define TIMEOUT (0x5000UL)

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

#define RX_BUF_SIZE 32 // must be a power of 2, for the masking trick below

typedef struct
{
    volatile uint8_t data[RX_BUF_SIZE];
    volatile uint8_t head; // next write index
    volatile uint8_t tail; // next read index
} RingBuffer;

static RingBuffer rx_buf = {0};

uint32_t APB1_GetClock(void)
{
    static const uint8_t ppre1_table[8] = {0, 0, 0, 0, 1, 2, 3, 4};
    // PPRE1 encoding: 0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16
    uint32_t ppre1   = (RCC_CFGR >> 10) & 0x7;
    uint32_t divisor = 1 << ppre1_table[ppre1];
    return SystemCoreClock / divisor;
}

static void TC_Error(void)
{
    while (1)
    {
    }
}

void USART2_SetBaudRate(uint32_t baud_rate)
{
    uint32_t pclk1 = APB1_GetClock();

    // USARTDIV * 16, as an integer, avoids floating point
    uint32_t usartdiv_x16 = (pclk1 + (baud_rate / 2)) / baud_rate;
    // (adding baud_rate/2 before dividing rounds to nearest instead of truncating)

    uint32_t mantissa = usartdiv_x16 / 16;
    uint32_t fraction = usartdiv_x16 % 16;

    USART2->BRR = (mantissa << 4) | (fraction & 0xF);
}

void uart_init(uint32_t baud_rate)
{

    GPIO_Config uart_cfg = {
        .mode      = GPIO_MODE_ALTERNATE,
        .otype     = GPIO_OTYPE_PUSH_PULL,
        .speed     = GPIO_SPEED_HIGH,
        .pupd      = GPIO_PUPD_NONE,
        .alternate = 7 // unused, not in alternate mode
    };
    GPIO_Init(GPIOA, 2, uart_cfg);
    GPIO_Init(GPIOA, 3, uart_cfg);

    RCC_APB1ENR |= (1 << 17); // bit 17 = USART2EN

    USART2_SetBaudRate(baud_rate);

    USART2->CR1 |= (1 << 3);  // TE
    USART2->CR1 |= (1 << 2);  // RE
    USART2->CR1 |= (1 << 13); // UE — enable last
    USART2->CR1 |= (1 << 5);  // RXNEIE (bit 5) - RX not empty interrupt enable
    NVIC_ISER1 |= (1 << 6);   // enable IRQ 38 (USART2)
}

void uart_write_byte(uint8_t byte)
{
    while (!(USART2->SR & (1 << 7)))
    {
        // wait for TXE
    }
    USART2->DR = byte;
}

void uart_write_str(const char *str)
{
    while (*str)
    {
        uart_write_byte((uint8_t)*str);
        str++;
    }

    uint32_t timeout = TIMEOUT;

    while (!(USART2->SR & (1 << 6)))
    {
        // wait for TC (bit 6) — transmission complete, after the loop
        if (--timeout == 0UL)
        {
            TC_Error();
        }
    }
}

void uart_write_u32(uint32_t value)
{
    char buf[11]; // max 10 digits for uint32_t + 1 null terminator
    int i = 0;

    if (value == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        while (value > 0)
        {
            buf[i++] = (value % 10) + '0'; // extract least significant digit
            value /= 10;
        }
    }

    // buf now holds digits in reverse order, e.g. "321" for input 123
    // print backwards, from i-1 down to 0
    while (i > 0)
    {
        i--;
        uart_write_byte((uint8_t)buf[i]);
    }

    uint32_t timeout = TIMEOUT;

    while (!(USART2->SR & (1 << 6)))
    {
        // wait for TC after the last digit

        if (--timeout == 0UL)
        {
            TC_Error();
        }
    }
}

void USART2_IRQHandler(void)
{
    if (USART2->SR & (1 << 5))
    { // RXNE set — a byte has arrived
        uint8_t byte = (uint8_t)USART2->DR;

        uint8_t next_head = (rx_buf.head + 1) & (RX_BUF_SIZE - 1);
        if (next_head != rx_buf.tail)
        {
            // buffer not full — store the byte
            rx_buf.data[rx_buf.head] = byte;
            rx_buf.head              = next_head;
        }
        // else: buffer full, byte is dropped (overrun) — see note below
    }
}

bool uart_read_byte(uint8_t *out)
{
    if (rx_buf.head == rx_buf.tail)
    {
        return false; // buffer empty, nothing to read
    }

    *out        = rx_buf.data[rx_buf.tail];
    rx_buf.tail = (rx_buf.tail + 1) & (RX_BUF_SIZE - 1);
    return true;
}
