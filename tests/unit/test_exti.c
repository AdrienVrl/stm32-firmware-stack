
#include "exti.h"
#include "mock_freertos.h"
#include "mock_registers.h"
#include "stdint.h"
#include "unity.h"
SemaphoreHandle_t xButtonSemaphore = (SemaphoreHandle_t)0x1234;
#define EXTI        ((EXTI_TypeDef *)&mock_exti)
#define NVIC_ISER1  mock_nvic_iser1
#define NVIC_IPR(n) mock_nvic_ipr[n]
void setUp()
{
    mock_registers_reset();
}

void tearDown()
{
}

void test_init(void)
{
    ButtonEXTI_Init();
    TEST_ASSERT_EQUAL_HEX32((1 << 2), mock_rcc_ahb1enr & (1 << 2));
    TEST_ASSERT_EQUAL_HEX32((1 << 14), mock_rcc_apb2enr & (1 << 14));
    TEST_ASSERT_EQUAL_HEX32((0x2 << 4), mock_syscfg_exticr & (0x3 << 4));
    TEST_ASSERT_EQUAL_HEX32((1 << 13), EXTI->FTSR & (1 << 13));
    TEST_ASSERT_EQUAL_HEX32(0, EXTI->RTSR & (1 << 13));
    TEST_ASSERT_EQUAL_HEX32((1 << 13), EXTI->IMR & (1 << 13));
    TEST_ASSERT_EQUAL_HEX32((1 << 8), NVIC_ISER1 & (1 << 8));
    TEST_ASSERT_EQUAL_HEX32(configMAX_SYSCALL_INTERRUPT_PRIORITY, NVIC_IPR(40));
}

void test_isr_gives_button_semaphore(void)
{
    EXTI->PR = (1 << 13);
    EXTI15_10_IRQHandler();
    TEST_ASSERT_EQUAL(1, mock_semaphore_give_call_count);
    TEST_ASSERT_EQUAL_PTR(xButtonSemaphore, mock_last_semaphore_given);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);
    RUN_TEST(test_isr_gives_button_semaphore);

    UNITY_END();

    return 0;
}
