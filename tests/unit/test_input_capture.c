

#include "input_capture.h"
#include "mock_registers.h"
#include "stdint.h"
#include "unity.h"
#define TIM3       ((TIM_TypeDef *)&mock_tim)
#define GPIOA      ((GPIO_Port *)&mock_gpioa)
#define NVIC_ISER0 mock_nvic_iser0

extern void TIM3_IRQHandler(void);
void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_init(void)
{
    input_capture_init();
    TEST_ASSERT_EQUAL_HEX32((1 << 1), mock_rcc_apb1enr & (1 << 1));
    TEST_ASSERT_EQUAL_HEX32(0x1, TIM3->CCMR1 & (0x3 << 0));
    TEST_ASSERT_EQUAL_HEX32(0, TIM3->CCMR1 & (0x3 << 2));
    TEST_ASSERT_EQUAL_HEX32(0, TIM3->CCMR1 & (0xF << 4));
    TEST_ASSERT_EQUAL_HEX32(0, TIM3->CCER & (1 << 1));
    TEST_ASSERT_EQUAL_HEX32(0, TIM3->CCER & (1 << 3));
    TEST_ASSERT_EQUAL_HEX32(1, TIM3->CCER & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 1), TIM3->DIER & (1 << 1));
    TEST_ASSERT_EQUAL_HEX32(0, TIM3->PSC);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF, TIM3->ARR);
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM3->EGR & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM3->CR1 & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 0), TIM3->DIER & (1 << 0));
    TEST_ASSERT_EQUAL_HEX32((1 << 29), NVIC_ISER0 & (1 << 29));
}

void test_frequency_no_capture_yet(void)
{
    input_capture_init();
    TEST_ASSERT_EQUAL(0, input_capture_get_frequency_hz());
}

void test_frequency_basic(void)
{
    input_capture_init();
    TIM3->SR |= (1 << 0);
    mock_tim.SR   = (1 << 1);
    mock_tim.CCR1 = 32000;
    TIM3_IRQHandler();

    TEST_ASSERT_EQUAL(16000000UL / 32000UL, input_capture_get_frequency_hz());
}

void test_frequency_overflow_correction(void)
{
    input_capture_init();

    mock_tim.SR   = (1 << 1);
    mock_tim.CCR1 = 60000;
    TIM3_IRQHandler();

    mock_tim.SR = (1 << 0); // UIF
    TIM3_IRQHandler();      // overflow_count -> 1

    mock_tim.SR   = (1 << 1);
    mock_tim.CCR1 = 100;
    TIM3_IRQHandler();

    // tick_diff = (uint16_t)(100 - 60000) = 5636 (wraparound-safe)
    // correct:  last_period_ticks = (1 - 1) * 0x10000 + 5636 = 5636
    // buggy:    last_period_ticks = (1)     * 0x10000 + 5636 = 71172  <- double-counted
    TEST_ASSERT_EQUAL(16000000UL / 5636UL, input_capture_get_frequency_hz());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_frequency_no_capture_yet);
    RUN_TEST(test_frequency_basic);
    RUN_TEST(test_frequency_overflow_correction);

    UNITY_END();

    return 0;
}
