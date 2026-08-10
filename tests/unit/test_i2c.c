#include "i2c.h"
#include "mock_registers.h"
#include "stdint.h"
#include "unity.h"
#define I2C1  ((I2C_TypeDef *)&mock_i2c)
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
    i2c_init(1000);
    TEST_ASSERT_EQUAL_HEX32((1 << 1), mock_rcc_ahb1enr & (1 << 1));
    TEST_ASSERT_EQUAL_HEX32((1 << 21), mock_rcc_apb1enr & (1 << 21));

    TEST_ASSERT_EQUAL_HEX32(0x10, I2C1->CR2 & (0x3F));
    TEST_ASSERT_EQUAL_HEX32(0xF40, I2C1->CCR & (0xFFF));
    TEST_ASSERT_EQUAL_HEX32(0x11, I2C1->TRISE & (0x3F));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), I2C1->CR1 & (1 << 0));
}

void test_init_fast(void)
{
    i2c_init(500000);
    TEST_ASSERT_EQUAL_HEX32((1 << 1), mock_rcc_ahb1enr & (1 << 1));
    TEST_ASSERT_EQUAL_HEX32((1 << 21), mock_rcc_apb1enr & (1 << 21));

    TEST_ASSERT_EQUAL_HEX32(0x10, I2C1->CR2 & (0x3F));
    TEST_ASSERT_EQUAL_HEX32(0xA, I2C1->CCR & (0xFFF));
    TEST_ASSERT_EQUAL_HEX32((1 << 15), I2C1->CCR & (1 << 15));
    TEST_ASSERT_EQUAL_HEX32(0x5, I2C1->TRISE & (0x3F));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), I2C1->CR1 & (1 << 0));
}

void test_init_switch(void)
{
    i2c_init(500000); // fast mode first
    i2c_init(1000);   // then standard mode
    TEST_ASSERT_EQUAL_HEX32(0, I2C1->CCR & ((1 << 15) | (1 << 14)));
}

void test_write(void)
{
    i2c_init(1000);
    I2C1->SR1 |= (1 << 0);
    I2C1->SR1 |= (1 << 1);
    I2C1->SR1 |= (1 << 7);
    I2C1->SR1 |= (1 << 2);
    uint8_t address = 0x68;
    uint8_t data[3] = {0x1, 0x2, 0x3};
    i2c_write(address, data, 3);
    TEST_ASSERT_EQUAL_HEX32((1 << 8), I2C1->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32(0x3, I2C1->DR);
    TEST_ASSERT_EQUAL_HEX32((1 << 9), I2C1->CR1 & (1 << 9));
}

void test_read1(void)
{
    i2c_init(1000);
    I2C1->SR1 |= (1 << 0);
    I2C1->SR1 |= (1 << 1);
    I2C1->SR1 |= (1 << 6);
    I2C1->SR1 |= (1 << 7);
    I2C1->SR1 |= (1 << 2);
    uint8_t address   = 0x68;
    uint8_t data[1]   = {0x1};
    uint8_t result[1] = {0};
    i2c_write(address, data, 1);
    i2c_read(address, result, 1);
    TEST_ASSERT_EQUAL_HEX32((1 << 8), I2C1->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32((1 << 9), I2C1->CR1 & (1 << 9));
    TEST_ASSERT_EQUAL_HEX32((0 << 10), I2C1->CR1 & (1 << 10));
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, I2C1->DR);
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, result[0]);
}

void test_read(void)
{
    i2c_init(1000);
    I2C1->SR1 |= (1 << 0);
    I2C1->SR1 |= (1 << 1);
    I2C1->SR1 |= (1 << 6);
    I2C1->SR1 |= (1 << 7);
    I2C1->SR1 |= (1 << 2);
    uint8_t address   = 0x68;
    uint8_t data[3]   = {0x1, 0x2, 0x3};
    uint8_t result[3] = {0, 0, 0};
    i2c_write(address, data, 3);
    i2c_read(address, result, 3);
    TEST_ASSERT_EQUAL_HEX32((1 << 8), I2C1->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32((1 << 9), I2C1->CR1 & (1 << 9));
    TEST_ASSERT_EQUAL_HEX32((0 << 10), I2C1->CR1 & (1 << 10));
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, I2C1->DR);
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, result[0]);
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, result[1]);
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, result[2]);
}

void test_write_reg(void)
{
    i2c_init(1000);
    I2C1->SR1 |= (1 << 0);
    I2C1->SR1 |= (1 << 1);
    I2C1->SR1 |= (1 << 7);
    I2C1->SR1 |= (1 << 2);
    uint8_t address = 0x68;
    uint8_t reg     = 0x7;
    uint8_t value   = 0x57;
    i2c_write_reg(address, reg, value);
    TEST_ASSERT_EQUAL_HEX32((1 << 8), I2C1->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32(0x57, I2C1->DR);
    TEST_ASSERT_EQUAL_HEX32((1 << 9), I2C1->CR1 & (1 << 9));
}

void test_read_reg(void)
{
    i2c_init(1000);
    I2C1->SR1 |= (1 << 0);
    I2C1->SR1 |= (1 << 1);
    I2C1->SR1 |= (1 << 6);
    I2C1->SR1 |= (1 << 7);
    I2C1->SR1 |= (1 << 2);
    uint8_t address = 0x68;
    uint8_t reg     = 0x7;
    uint8_t value   = 0x57;
    i2c_write_reg(address, reg, value);
    uint8_t result = i2c_read_reg(address, reg);
    TEST_ASSERT_EQUAL_HEX32((1 << 8), I2C1->CR1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32((1 << 9), I2C1->CR1 & (1 << 9));
    TEST_ASSERT_EQUAL_HEX32((0 << 10), I2C1->CR1 & (1 << 10));
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, I2C1->DR);
    TEST_ASSERT_EQUAL_HEX32((0x68 << 1) | 1, result);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_init_fast);
    RUN_TEST(test_init_switch);
    RUN_TEST(test_write);
    RUN_TEST(test_read1);
    RUN_TEST(test_read);
    RUN_TEST(test_write_reg);
    RUN_TEST(test_read_reg);

    UNITY_END();

    return 0;
}
