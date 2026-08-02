#include "exti.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "gpio.h"
#include "semphr.h"

#include <stdint.h>

#define RCC_BASE 0x40023800
#define RCC_AHB1ENR                                                            \
  (*(volatile uint32_t *)(RCC_BASE +                                           \
                          0x30)) // add 0x30 offset for AHB1ENR register
#define RCC_APB2ENR                                                            \
  (*(volatile uint32_t *)(RCC_BASE +                                           \
                          0x44)) // add 0x44 offset for APB2ENR register
#define GPIOC_BASE 0x40020800UL
#define GPIOC ((GPIO_Port *)(GPIOC_BASE))
#define EXTI_BASE 0x40013C00UL
#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

#define SYSCFG_EXTICR_BASE 0x40013814
#define SYSCFG_EXTICR (*(volatile uint32_t *)SYSCFG_EXTICR_BASE)

#define NVIC_ISER1 (*(volatile uint32_t *)0xE000E104UL)

#define NVIC_IPR_BASE 0xE000E400UL
#define NVIC_IPR(n) (*(volatile uint8_t *)(NVIC_IPR_BASE + (n)))

void ButtonEXTI_Init(void) {
  /* Enable clocks: GPIOC for the pin, SYSCFG for EXTI line routing */
  RCC_AHB1ENR |= (1 << 2);
  RCC_APB2ENR |= (1 << 14);

  GPIO_Config cfg = {.mode = GPIO_MODE_INPUT,
                     .otype = GPIO_OTYPE_PUSH_PULL,
                     .speed = GPIO_SPEED_LOW,
                     .pupd = GPIO_PUPD_PULLUP,
                     .alternate = 0};
  GPIO_Init(GPIOC, 13, cfg);

  SYSCFG_EXTICR &= ~(0xF << 4);
  SYSCFG_EXTICR |= (0x2 << 4); // 0010 = GPIOC

  EXTI->FTSR |= (1 << 13);
  EXTI->RTSR &= ~(1 << 13);

  EXTI->IMR |= (1 << 13);

  NVIC_ISER1 |= (1 << 8); // Unable IRQ
  NVIC_IPR(40) = configMAX_SYSCALL_INTERRUPT_PRIORITY;
}

extern SemaphoreHandle_t xButtonSemaphore;

void EXTI15_10_IRQHandler(void) {
  if (EXTI->PR & (1 << 13)) {
    EXTI->PR = (1 << 13); // clear pending bit: write 1 to clear

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
