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
#include <system_stm32f4xx.h>

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

/*-----------------------------------------------------------------------*/
/* Clock Control Register (RCC).                                           */
/*-----------------------------------------------------------------------*/
#define RCC_BASE  (0x40023800UL)
#define RCC_CR    (*(volatile uint32_t *)(RCC_BASE + 0x00UL))
#define HSE_VALUE (8000000UL)

#define RCC_CR_HSEON  (1UL << 16)
#define RCC_CR_HSERDY (1UL << 17)
#define RCC_CR_HSEBYP (1UL << 18)
#define TIMEOUT       (0x5000UL)

static void HSE_ConfigError(void)
{
    while (1)
    {
    }
}

static void PLL_ConfigError(void)
{
    while (1)
    {
    }
}

/*-----------------------------------------------------------------------*/
/* Flash latency                                                         */
/*-----------------------------------------------------------------------*/

#define FLASH_R_BASE (0x40023C00UL)
#define FLASH_ACR    (*(volatile uint32_t *)(FLASH_R_BASE + 0x00UL))

#define FLASH_ACR_LATENCY_Msk (0xFUL << 0)
#define FLASH_ACR_LATENCY_5WS (5UL << 0)
#define FLASH_ACR_PRFTEN      (1UL << 8)
#define FLASH_ACR_ICEN        (1UL << 9)
#define FLASH_ACR_DCEN        (1UL << 10)

/*-----------------------------------------------------------------------*/
/* PLL_PLLCFGR                                                           */
/*-----------------------------------------------------------------------*/

#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x04UL))

#define PLL_M 4U // PLL_input = HSE / PLLM -> PLLM = HSE / PLL_input = 8 MHz / 2 MHz = 4
#define PLL_P 2U // VCO_output = SYSCLK * PLL_P = 180 MHz * 2 = 360, and 100 <= 360 <= 420
#define PLL_Q 7U // PLL_Q = VCO_output / 48 MHz = 360 MHz / 48 MHz = 7.5, round to 7
#define PLL_N                                                                                      \
    180U // VCO_output = PLL_input * PLLN -> PLLN = VCO_output / PLL_input = 360 MHz / 2 MHz = 180

#define RCC_PLLCFGR_PLLM_Pos   0U
#define RCC_PLLCFGR_PLLN_Pos   6U
#define RCC_PLLCFGR_PLLP_Pos   16U
#define RCC_PLLCFGR_PLLSRC_Pos 22U
#define RCC_PLLCFGR_PLLQ_Pos   24U

#define RCC_PLLCFGR_PLLSRC_HSE (1UL << RCC_PLLCFGR_PLLSRC_Pos)
#define PLL_P_ENCODED          ((PLL_P / 2U) - 1U)

/*-----------------------------------------------------------------------*/
/* PLL_CFGR                                                              */
/*-----------------------------------------------------------------------*/

#define RCC_CFGR (*(volatile uint32_t *)(RCC_BASE + 0x08UL))

#define RCC_CR_PLLON  (1UL << 24)
#define RCC_CR_PLLRDY (1UL << 25)

#define RCC_CFGR_SW_Pos 0U
#define RCC_CFGR_SW_Msk (0x3UL << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_PLL (0x2UL << RCC_CFGR_SW_Pos)

#define RCC_CFGR_SWS_Pos 2U
#define RCC_CFGR_SWS_Msk (0x3UL << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL (0x2UL << RCC_CFGR_SWS_Pos)

#define RCC_CFGR_HPRE_Pos  4U
#define RCC_CFGR_HPRE_Msk  (0xFUL << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_HPRE_DIV1 (0x0UL << RCC_CFGR_HPRE_Pos) // 180 MHz / 1 = 180 MHz = AHB_max

#define RCC_CFGR_PPRE1_Pos  10U
#define RCC_CFGR_PPRE1_Msk  (0x7UL << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE1_DIV4 (0x5UL << RCC_CFGR_PPRE1_Pos) // 180 MHz / 4 = 45 MHz = APB1_max

#define RCC_CFGR_PPRE2_Pos  13U
#define RCC_CFGR_PPRE2_Msk  (0x7UL << RCC_CFGR_PPRE2_Pos)
#define RCC_CFGR_PPRE2_DIV2 (0x4UL << RCC_CFGR_PPRE2_Pos) // 180 MHz / 2 = 90 MHz = APB2_max

/*-----------------------------------------------------------------------*/
/* PWR                                                                   */
/*-----------------------------------------------------------------------*/

#define RCC_BASE          (0x40023800UL)
#define RCC_APB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x40UL))
#define RCC_APB1ENR_PWREN (1UL << 28)

#define PWR_BASE (0x40007000UL)
#define PWR_CR   (*(volatile uint32_t *)(PWR_BASE + 0x00UL))
#define PWR_CSR  (*(volatile uint32_t *)(PWR_BASE + 0x04UL))

#define PWR_CR_VOS_Pos    14U
#define PWR_CR_VOS_Msk    (0x3UL << PWR_CR_VOS_Pos)
#define PWR_CR_VOS_SCALE1 (0x3UL << PWR_CR_VOS_Pos) /* 0b11 */

#define PWR_CR_ODEN   (1UL << 16)
#define PWR_CR_ODSWEN (1UL << 17)

#define PWR_CSR_ODRDY   (1UL << 16)
#define PWR_CSR_ODSWRDY (1UL << 17)


// global system clock
uint32_t SystemCoreClock = 16000000UL;

void SystemClock_Config(void)
{
    // enable HSE and wait until it is ready
    uint32_t timeout = TIMEOUT;
    RCC_CR |= RCC_CR_HSEBYP;
    RCC_CR |= RCC_CR_HSEON;

    while (!(RCC_CR & RCC_CR_HSERDY))
    {
        if (--timeout == 0UL)
        {
            HSE_ConfigError();
        }
    }

    // configure flash latency to 5WS and wait for acknowledgement
    FLASH_ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;

    while ((FLASH_ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_5WS)
    {
    }

    // enable PWR clock
    volatile uint32_t dummy;
    uint32_t cr;

    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    dummy = RCC_APB1ENR;
    (void)dummy;

    // select scale 1 voltage regulation
    cr = PWR_CR;
    cr &= ~PWR_CR_VOS_Msk;
    cr |= PWR_CR_VOS_SCALE1;
    PWR_CR = cr;

    // enable Over-Drive mode
    PWR_CR |= PWR_CR_ODEN;
    while (!(PWR_CSR & PWR_CSR_ODRDY))
    {
    }

    PWR_CR |= PWR_CR_ODSWEN;
    while (!(PWR_CSR & PWR_CSR_ODSWRDY))
    {
    }

    // configure PLL
    RCC_PLLCFGR = (PLL_M << RCC_PLLCFGR_PLLM_Pos) | (PLL_N << RCC_PLLCFGR_PLLN_Pos) |
                  (PLL_P_ENCODED << RCC_PLLCFGR_PLLP_Pos) | RCC_PLLCFGR_PLLSRC_HSE |
                  (PLL_Q << RCC_PLLCFGR_PLLQ_Pos);

    // configure prescalers
    uint32_t cfgr = RCC_CFGR;

    cfgr &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk);
    cfgr |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

    RCC_CFGR = cfgr;

    // enable PLL and wait for acknowledgement
    timeout = TIMEOUT;
    RCC_CR |= RCC_CR_PLLON;

    while (!(RCC_CR & RCC_CR_PLLRDY))
    {
        if (--timeout == 0UL)
        {
            PLL_ConfigError();
        }
    }

    // set SW and wait for acknowledgement
    cfgr = RCC_CFGR;
    cfgr &= ~RCC_CFGR_SW_Msk;
    cfgr |= RCC_CFGR_SW_PLL;
    RCC_CFGR = cfgr;

    while ((RCC_CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL)
    {
    }

    SystemCoreClock = (HSE_VALUE / PLL_M) * PLL_N / PLL_P; // SYSCLK = (HSE / PLL_M) * PLL_N / PLL_P
}
