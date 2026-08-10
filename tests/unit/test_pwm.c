
#include "mock_registers.h"
#include "pwm.h"
#include "stdint.h"
#include "unity.h"
#define TIM2  ((TIM_TypeDef *)&mock_tim)
#define GPIOA ((GPIO_Port *)&mock_gpioa)


void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_init(void)
{
    pwm_init(1000);
    TEST_ASSERT_EQUAL_HEX32((1 << 0), mock_rcc_apb1enr & (1 << 0));

    TEST_ASSERT_EQUAL_HEX32(0, TIM2->PSC);
    TEST_ASSERT_EQUAL_HEX32(0x3E7F, TIM2->ARR);
    TEST_ASSERT_EQUAL_HEX32((1 << 3), TIM2->CCMR1 & (1 << 3));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM2->CCER & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 7), TIM2->CR1 & (1 << 7));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM2->EGR & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM2->CR1 & (1 << 0));
}

void test_set_duty(void)
{
    pwm_init(1000);
    pwm_set_duty(50);
    TEST_ASSERT_EQUAL_HEX32(0x1F3F, TIM2->CCR1);
}

void test_set_duty_over_100(void)
{
    pwm_init(1000);
    pwm_set_duty(150);
    TEST_ASSERT_EQUAL_HEX32(0x3E7F, TIM2->CCR1);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_set_duty);
    RUN_TEST(test_set_duty_over_100);

    UNITY_END();

    return 0;
}
