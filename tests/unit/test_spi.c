
#include "mock_registers.h"
#include "spi.h"
#include "stdint.h"
#include "unity.h"
#define SPI2  ((SPI_TypeDef *)&mock_spi)
#define GPIOB ((GPIO_Port *)&mock_gpioa)


void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_init(void)
{
    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};
    spi_init(cfg);
    TEST_ASSERT_EQUAL_HEX32((1 << 14), mock_rcc_apb1enr);

    TEST_ASSERT_EQUAL_HEX32(SPI_MODE0 & 0x3, SPI2->CR1 & (0x3));
    TEST_ASSERT_EQUAL_HEX32(SPI_PRESCALER_4 << 3, SPI2->CR1 & (0x1 << 3));
    TEST_ASSERT_EQUAL_HEX32((1 << 2), SPI2->CR1 & (1 << 2));
    TEST_ASSERT_EQUAL_HEX32((1 << 8), SPI2->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32((1 << 9), SPI2->CR1 & (1 << 9));
    TEST_ASSERT_EQUAL_HEX32((1 << 6), SPI2->CR1 & (1 << 6));
}

void test_init2(void)
{
    SPI_Config cfg = {.mode = SPI_MODE3, .prescaler = SPI_PRESCALER_128};
    spi_init(cfg);
    TEST_ASSERT_EQUAL_HEX32((1 << 14), mock_rcc_apb1enr);

    TEST_ASSERT_EQUAL_HEX32(SPI_MODE3 & 0x3, SPI2->CR1 & (0x3));
    TEST_ASSERT_EQUAL_HEX32(SPI_PRESCALER_128 << 3, SPI2->CR1 & (0x7 << 3));
    TEST_ASSERT_EQUAL_HEX32((1 << 2), SPI2->CR1 & (1 << 2));
    TEST_ASSERT_EQUAL_HEX32((1 << 8), SPI2->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32((1 << 9), SPI2->CR1 & (1 << 9));
    TEST_ASSERT_EQUAL_HEX32((1 << 6), SPI2->CR1 & (1 << 6));
}

void test_transfer_byte(void)
{

    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};
    spi_init(cfg);
    SPI2->SR |= (1 << 0);
    SPI2->SR |= (1 << 1);
    uint8_t result = spi_transfer_byte(0xA5);
    TEST_ASSERT_EQUAL_HEX8(0xA5, mock_spi.DR);
    TEST_ASSERT_EQUAL_HEX8(0xA5, result);
}

void test_write(void)
{

    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};
    spi_init(cfg);
    SPI2->SR |= (1 << 0);
    SPI2->SR |= (1 << 1);
    uint8_t data[2] = {0xA5, 0xB7};
    spi_write(data, 2);
    TEST_ASSERT_EQUAL_HEX8(0xB7, mock_spi.DR);
}

void test_read(void)
{

    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};
    spi_init(cfg);
    SPI2->SR |= (1 << 0);
    SPI2->SR |= (1 << 1);
    uint8_t result[3] = {0};
    spi_read(result, 3);
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL_HEX8(0xFF, result[i]);
}

void test_read_cs_timing(void)
{
    SPI_Config cfg = {.mode = SPI_MODE0, .prescaler = SPI_PRESCALER_4};
    spi_init(cfg);
    SPI2->SR |= (1 << 0);
    SPI2->SR |= (1 << 1);
    uint8_t data[1] = {0};

    spi_read(data, 1);

    TEST_ASSERT_TRUE(GPIOB->ODR & (1 << 12));
}


int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_init2);
    RUN_TEST(test_transfer_byte);
    RUN_TEST(test_read);
    RUN_TEST(test_read_cs_timing);
    RUN_TEST(test_write);

    UNITY_END();

    return 0;
}
