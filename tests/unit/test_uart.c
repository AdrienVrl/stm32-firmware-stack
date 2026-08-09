#include "mock_registers.h"
#include "stdint.h"
#include "uart.h"
#include "unity.h"
#define USART2 ((USART_Port *)&mock_usart2)
#define GPIOA  ((GPIO_Port *)&mock_gpioa)
extern void USART2_IRQHandler(void);


void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_GetClock(void)
{
    TEST_ASSERT_EQUAL(APB1_GetClock(), 16000000);
}

void test_GetClock_ppre1_div2(void)
{
    mock_rcc_cfgr = (0x4 << 10); // PPRE1 = 100 -> /2
    TEST_ASSERT_EQUAL(8000000, APB1_GetClock());
}

void test_GetClock_ppre1_div8(void)
{
    mock_rcc_cfgr = (0x6 << 10); // PPRE1 = 110 -> /8
    TEST_ASSERT_EQUAL(2000000, APB1_GetClock());
}
void test_init(void)
{
    uart_init(115200);
    TEST_ASSERT_EQUAL_HEX32((1 << 17), mock_rcc_apb1enr);

    TEST_ASSERT_EQUAL_HEX32((8 << 4) | (11 & 0xF), USART2->BRR);

    TEST_ASSERT_EQUAL_HEX32((1 << 3), USART2->CR1 & (1 << 3));
    TEST_ASSERT_EQUAL_HEX32((1 << 2), USART2->CR1 & (1 << 2));
    TEST_ASSERT_EQUAL_HEX32((1 << 13), USART2->CR1 & (1 << 13));
    TEST_ASSERT_EQUAL_HEX32((1 << 5), USART2->CR1 & (1 << 5));
    TEST_ASSERT_EQUAL_HEX32((1 << 6), mock_nvic_iser1 & (1 << 6));

    TEST_ASSERT_EQUAL(GPIO_MODE_ALTERNATE, (GPIOA->MODER >> (2 * 2)) & 0x3);
    TEST_ASSERT_EQUAL(7, (GPIOA->AFR[0] >> (2 * 4)) & 0xF);
    TEST_ASSERT_EQUAL(GPIO_MODE_ALTERNATE, (GPIOA->MODER >> (3 * 2)) & 0x3);
    TEST_ASSERT_EQUAL(7, (GPIOA->AFR[0] >> (3 * 4)) & 0xF);
}

void test_write(void)
{
    uart_init(115200);
    uart_write_byte(173);
    USART2->SR |= (1 << 7);
    TEST_ASSERT_EQUAL(173, USART2->DR);
}

void test_write_str(void)
{
    uart_init(115200);
    char data[2] = {(char)173, (char)187};
    uart_write_str(data);
    TEST_ASSERT_EQUAL(187, USART2->DR);
}

void test_write_u32(void)
{
    uart_init(115200);
    uint32_t data = 262143;
    uart_write_u32(data);
    TEST_ASSERT_EQUAL(51, USART2->DR);
}

void test_write_u32_0(void)
{

    uart_init(115200);
    uint32_t data = 0;
    uart_write_u32(data);
    TEST_ASSERT_EQUAL('0', USART2->DR);
}

void test_read_byte_after_interrupt(void)
{
    uart_init(115200);
    mock_usart2.SR = (1 << 5); // RXNE
    mock_usart2.DR = 0x42;

    USART2_IRQHandler(); // simulates the byte having arrived

    uint8_t out = 0;
    TEST_ASSERT_TRUE(uart_read_byte(&out));
    TEST_ASSERT_EQUAL_HEX8(0x42, out);
}

void test_read_byte_empty_buffer(void)
{
    uart_init(115200);
    uint8_t out = 0xFF;
    TEST_ASSERT_FALSE(uart_read_byte(&out));
}

void test_read_byte_overrun_buffer(void)
{
    mock_usart2.SR = (1 << 5); // RXNE
    for (int i = 0; i < 32; i++)
    {
        mock_usart2.DR = i;

        USART2_IRQHandler();
    }

    uint8_t out = 0;
    for (int i = 0; i < 31; i++)
    {
        TEST_ASSERT_TRUE(uart_read_byte(&out));
        TEST_ASSERT_EQUAL(i, out);
    }
    TEST_ASSERT_FALSE(uart_read_byte(&out));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_GetClock);
    RUN_TEST(test_GetClock_ppre1_div2);
    RUN_TEST(test_GetClock_ppre1_div8);
    RUN_TEST(test_init);
    RUN_TEST(test_write);
    RUN_TEST(test_write_str);
    RUN_TEST(test_write_u32);
    RUN_TEST(test_write_u32_0);
    RUN_TEST(test_read_byte_after_interrupt);
    RUN_TEST(test_read_byte_empty_buffer);
    RUN_TEST(test_read_byte_overrun_buffer);

    UNITY_END();

    return 0;
}
