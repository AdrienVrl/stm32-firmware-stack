/******************************************************************************
 * system_stm32f4xx.c
 *
 * SystemInit() responsibilities at this stage:
 *   1. Enable the FPU coprocessor (CP10/CP11 full access via SCB->CPACR).
 *   2. Explicitly set SCB->VTOR to the vector table's base flash address.
 *
 * Clock tree configuration (HSE/PLL, flash wait states, bus prescalers)
 * is deliberately NOT done here -- that's a separate, later ticket. The
 * core runs on its reset-default HSI oscillator until then.
 *****************************************************************************/

#include <stdint.h>

/*-----------------------------------------------------------------------*/
/* Symbol from startup.c: the actual base address of the vector table,   */
/* wherever the linker placed the .isr_vector section. Referencing it    */
/* directly (rather than hardcoding 0x08000000) means SCB->VTOR always   */
/* tracks the real placement, even if FLASH's origin or the section's    */
/* position within the image ever changes.                              */
/*-----------------------------------------------------------------------*/
extern const uint8_t g_pfnVectors[];

/*-----------------------------------------------------------------------*/
/* System Control Block (SCB) registers.                                 */
/* Addresses per the ARMv7-M Architecture Reference Manual, B3.2.2.       */
/*-----------------------------------------------------------------------*/
#define SCB_BASE  (0xE000ED00UL)
#define SCB_VTOR  (*(volatile uint32_t *)(SCB_BASE + 0x08UL))
#define SCB_CPACR (*(volatile uint32_t *)(SCB_BASE + 0x88UL))

void SystemInit(void)
{
    /*---------------------------------------------------------------
     * Enable the FPU coprocessor: CP10 (bits [21:20]) and CP11
     * (bits [23:22]) both set to 0b11 = full access (privileged and
     * unprivileged). Every source file in this project compiles with
     * -mfloat-abi=hard -mfpu=fpv4-sp-d16, so ANY floating-point
     * instruction anywhere in the final binary -- including ones the
     * compiler generates on its own later (struct copies, printf
     * argument promotion, etc.), not just float math written by
     * hand -- takes a NOCP UsageFault until this bit is set.
     *--------------------------------------------------------------*/
    SCB_CPACR |= (0xFUL << 20);

    /* Barriers: the FPU enable doesn't take effect instantaneously
     * due to pipelining. Without these, an instruction fetched
     * immediately after the CPACR write could still fault if it
     * happens to be floating point. This is the ARM-recommended
     * sequence, not optional margin. */
    __asm volatile("dsb 0xF" ::: "memory");
    __asm volatile("isb 0xF" ::: "memory");

    /*---------------------------------------------------------------
     * Vector table base address. Functionally a no-op right now:
     * SCB->VTOR resets to 0x00000000, and the STM32's boot-pin
     * aliasing already mirrors flash at address 0, so the core finds
     * the same table either way. Set it explicitly regardless, so
     * this stays correct later if the table is ever relocated to
     * SRAM, or if a bootloader hands off execution with VTOR already
     * pointing elsewhere.
     *--------------------------------------------------------------*/
    SCB_VTOR = (uint32_t)g_pfnVectors;
}
