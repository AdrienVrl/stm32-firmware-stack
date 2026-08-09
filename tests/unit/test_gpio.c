#include "gpio.h"
#include "mock_registers.h"
#include "stdint.h"
#include "unity.h"
#define GPIOA ((GPIO_Port *)&mock_gpioa)

GPIO_Config cfg = {
    .mode      = GPIO_MODE_OUTPUT,
    .otype     = GPIO_OTYPE_PUSH_PULL,
    .speed     = GPIO_SPEED_LOW,
    .pupd      = GPIO_PUPD_NONE,
    .alternate = 0 // unused, not in alternate mode
};

GPIO_Config cfg_alt = {.mode      = GPIO_MODE_ALTERNATE,
                       .otype     = GPIO_OTYPE_PUSH_PULL,
                       .speed     = GPIO_SPEED_LOW,
                       .pupd      = GPIO_PUPD_PULLUP,
                       .alternate = 4};

GPIO_Config cfg_alt2 = {.mode      = GPIO_MODE_ALTERNATE,
                        .otype     = GPIO_OTYPE_PUSH_PULL,
                        .speed     = GPIO_SPEED_LOW,
                        .pupd      = GPIO_PUPD_PULLUP,
                        .alternate = 4};

void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_init(void)
{
    GPIO_Init(GPIOA, 8, cfg);
    TEST_ASSERT_EQUAL_HEX32(1 << 0, mock_rcc_ahb1enr);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_OUTPUT << (8 * 2), GPIOA->MODER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_PUSH_PULL << 8, GPIOA->OTYPER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_SPEED_LOW << (8 * 2), GPIOA->OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_NONE << (8 * 2), GPIOA->PUPDR);
}

void test_init_alternate(void)
{
    GPIO_Init(GPIOA, 2, cfg_alt);
    TEST_ASSERT_EQUAL_HEX32(1 << 0, mock_rcc_ahb1enr);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_ALTERNATE << (2 * 2), GPIOA->MODER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_PUSH_PULL << 2, GPIOA->OTYPER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_SPEED_LOW << (2 * 2), GPIOA->OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_PULLUP << (2 * 2), GPIOA->PUPDR);
    TEST_ASSERT_EQUAL_HEX32(4 << 8, GPIOA->AFR[0]);
}

void test_init_alternate2(void)
{
    GPIO_Init(GPIOA, 10, cfg_alt2);
    TEST_ASSERT_EQUAL_HEX32(1 << 0, mock_rcc_ahb1enr);
    TEST_ASSERT_EQUAL_HEX32(GPIO_MODE_ALTERNATE << (10 * 2), GPIOA->MODER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_OTYPE_PUSH_PULL << 10, GPIOA->OTYPER);
    TEST_ASSERT_EQUAL_HEX32(GPIO_SPEED_LOW << (10 * 2), GPIOA->OSPEEDR);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PUPD_PULLUP << (10 * 2), GPIOA->PUPDR);
    TEST_ASSERT_EQUAL_HEX32(4 << 8, GPIOA->AFR[1]);
}

void test_write(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 1);
    TEST_ASSERT_EQUAL_HEX32(1 << 2, GPIOA->ODR);
}

void test_clear(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 0);
    TEST_ASSERT_EQUAL_HEX32(0 << 2, GPIOA->ODR);
}

void test_toggle_high(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_TogglePin(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(1 << 2, GPIOA->ODR);
}

void test_toggle_low(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 1);
    GPIO_TogglePin(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(0 << 2, GPIOA->ODR);
}

void test_toggle_bssr_high(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_TogglePinBSSR(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(1 << 2, GPIOA->BSRR);
}

void test_toggle_bssr_low(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 1);
    GPIO_TogglePinBSSR(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(1 << 2 << 16, GPIOA->BSRR);
}

void test_read(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 1);
    GPIOA->IDR          = 1 << 2;
    GPIO_PinState state = GPIO_ReadPin(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PIN_SET, state);
}

void test_read_reset(void)
{
    GPIO_Init(GPIOA, 2, cfg);
    GPIO_WritePin(GPIOA, 2, 0);
    GPIOA->IDR          = 0 << 2;
    GPIO_PinState state = GPIO_ReadPin(GPIOA, 2);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PIN_RESET, state);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_init_alternate);
    RUN_TEST(test_init_alternate2);
    RUN_TEST(test_write);
    RUN_TEST(test_clear);
    RUN_TEST(test_toggle_high);
    RUN_TEST(test_toggle_low);
    RUN_TEST(test_toggle_bssr_high);
    RUN_TEST(test_toggle_bssr_low);
    RUN_TEST(test_read);
    RUNTEST(test_read_reset);

    UNITY_END();

    return 0;
}
