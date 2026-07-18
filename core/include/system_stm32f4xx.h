#ifndef SYSTEM_STM32F4XX_H
#define SYSTEM_STM32F4XX_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  Early hardware bring-up, called from Reset_Handler before main().
     *
     * Current responsibilities:
     *   - Enable the FPU coprocessor (CP10/CP11 full access).
     *   - Set SCB->VTOR to the vector table's actual base address.
     *
     * Deliberately does NOT configure the clock tree yet (HSE/PLL, flash
     * wait states, bus prescalers) -- the core runs on its reset-default
     * HSI oscillator until that's implemented as its own ticket.
     */
    void SystemInit(void);

    extern uint32_t SystemCoreClock;

    void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STM32F4XX_H */
