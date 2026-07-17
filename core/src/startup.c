/******************************************************************************
 * startup.c
 * STM32F446xE startup code: vector table + Reset_Handler + default handlers.
 *
 * Vector table layout follows RM0390 (STM32F446xx reference manual),
 * Table "Vector table for STM32F446xx".
 *****************************************************************************/

#include <stdint.h>
#include <system_stm32f4xx.h>

/*-----------------------------------------------------------------------*/
/* Symbols provided by the linker script (see linker.ld)                 */
/*-----------------------------------------------------------------------*/
extern uint32_t _sidata; /* Flash: start of .data init values (LMA)     */
extern uint32_t _sdata;  /* SRAM:  start of .data (VMA)                 */
extern uint32_t _edata;  /* SRAM:  end of .data                         */
extern uint32_t _sbss;   /* SRAM:  start of .bss                        */
extern uint32_t _ebss;   /* SRAM:  end of .bss                          */
extern uint32_t _estack; /* SRAM:  top of stack (initial SP value)      */

/* Supplied by application / CMSIS */
extern int main(void);

/*-----------------------------------------------------------------------*/
/* Reset handler -- not weak: this is the real, single implementation.   */
/*-----------------------------------------------------------------------*/
void Reset_Handler(void);

/*-----------------------------------------------------------------------*/
/* Default handler -- catch-all target for every weak IRQ/exception      */
/* alias below. A strong definition of e.g. TIM2_IRQHandler anywhere     */
/* else in the project silently overrides its entry here at link time,  */
/* with zero changes required in this file.                             */
/*-----------------------------------------------------------------------*/
void Default_Handler(void)
{
    while (1)
    {
        /* Unhandled interrupt / exception. Attach a debugger and check
         * the active exception number in IPSR / the NVIC's active
         * register to identify which vector landed here. */
    }
}

/*-----------------------------------------------------------------------*/
/* HardFault_Handler -- deliberately NOT aliased to Default_Handler.     */
/* Its own infinite loop makes it instantly distinguishable from "some   */
/* other unhandled IRQ fired" when halted in a debugger: PC sitting in   */
/* THIS function means a hard fault, full stop. Stacked-register         */
/* unwinding / fault-status-register decoding is left for a later        */
/* ticket -- this is just the trap.                                     */
/*-----------------------------------------------------------------------*/
void HardFault_Handler(void)
{
    while (1)
    {
        /* Hard fault. Check SCB->HFSR, SCB->CFSR, and the stacked PC/LR
         * (via the stack pointer at time of entry) for root cause. */
    }
}

/*-----------------------------------------------------------------------*/
/* Weak aliases: every other core exception and every device interrupt.  */
/* WEAK_ALIAS handlers all resolve to Default_Handler unless overridden  */
/* by a strong definition elsewhere in the project.                     */
/*-----------------------------------------------------------------------*/
#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))

/* --- Cortex-M4 core exceptions --- */
void NMI_Handler(void) WEAK_ALIAS;
void MemManage_Handler(void) WEAK_ALIAS;
void BusFault_Handler(void) WEAK_ALIAS;
void UsageFault_Handler(void) WEAK_ALIAS;
void SVC_Handler(void) WEAK_ALIAS;
void DebugMon_Handler(void) WEAK_ALIAS;
void PendSV_Handler(void) WEAK_ALIAS;
void SysTick_Handler(void) WEAK_ALIAS;

/* --- STM32F446xx device interrupts (IRQ0 .. IRQ96) --- */
void WWDG_IRQHandler(void) WEAK_ALIAS;
void PVD_IRQHandler(void) WEAK_ALIAS;
void TAMP_STAMP_IRQHandler(void) WEAK_ALIAS;
void RTC_WKUP_IRQHandler(void) WEAK_ALIAS;
void FLASH_IRQHandler(void) WEAK_ALIAS;
void RCC_IRQHandler(void) WEAK_ALIAS;
void EXTI0_IRQHandler(void) WEAK_ALIAS;
void EXTI1_IRQHandler(void) WEAK_ALIAS;
void EXTI2_IRQHandler(void) WEAK_ALIAS;
void EXTI3_IRQHandler(void) WEAK_ALIAS;
void EXTI4_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream0_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream1_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream2_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream3_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream4_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream5_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream6_IRQHandler(void) WEAK_ALIAS;
void ADC_IRQHandler(void) WEAK_ALIAS;
void CAN1_TX_IRQHandler(void) WEAK_ALIAS;
void CAN1_RX0_IRQHandler(void) WEAK_ALIAS;
void CAN1_RX1_IRQHandler(void) WEAK_ALIAS;
void CAN1_SCE_IRQHandler(void) WEAK_ALIAS;
void EXTI9_5_IRQHandler(void) WEAK_ALIAS;
void TIM1_BRK_TIM9_IRQHandler(void) WEAK_ALIAS;
void TIM1_UP_TIM10_IRQHandler(void) WEAK_ALIAS;
void TIM1_TRG_COM_TIM11_IRQHandler(void) WEAK_ALIAS;
void TIM1_CC_IRQHandler(void) WEAK_ALIAS;
void TIM2_IRQHandler(void) WEAK_ALIAS;
void TIM3_IRQHandler(void) WEAK_ALIAS;
void TIM4_IRQHandler(void) WEAK_ALIAS;
void I2C1_EV_IRQHandler(void) WEAK_ALIAS;
void I2C1_ER_IRQHandler(void) WEAK_ALIAS;
void I2C2_EV_IRQHandler(void) WEAK_ALIAS;
void I2C2_ER_IRQHandler(void) WEAK_ALIAS;
void SPI1_IRQHandler(void) WEAK_ALIAS;
void SPI2_IRQHandler(void) WEAK_ALIAS;
void USART1_IRQHandler(void) WEAK_ALIAS;
void USART2_IRQHandler(void) WEAK_ALIAS;
void USART3_IRQHandler(void) WEAK_ALIAS;
void EXTI15_10_IRQHandler(void) WEAK_ALIAS;
void RTC_Alarm_IRQHandler(void) WEAK_ALIAS;
void OTG_FS_WKUP_IRQHandler(void) WEAK_ALIAS;
void TIM8_BRK_TIM12_IRQHandler(void) WEAK_ALIAS;
void TIM8_UP_TIM13_IRQHandler(void) WEAK_ALIAS;
void TIM8_TRG_COM_TIM14_IRQHandler(void) WEAK_ALIAS;
void TIM8_CC_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream7_IRQHandler(void) WEAK_ALIAS;
void FMC_IRQHandler(void) WEAK_ALIAS;
void SDIO_IRQHandler(void) WEAK_ALIAS;
void TIM5_IRQHandler(void) WEAK_ALIAS;
void SPI3_IRQHandler(void) WEAK_ALIAS;
void UART4_IRQHandler(void) WEAK_ALIAS;
void UART5_IRQHandler(void) WEAK_ALIAS;
void TIM6_DAC_IRQHandler(void) WEAK_ALIAS;
void TIM7_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream0_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream1_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream2_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream3_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream4_IRQHandler(void) WEAK_ALIAS;
void CAN2_TX_IRQHandler(void) WEAK_ALIAS;
void CAN2_RX0_IRQHandler(void) WEAK_ALIAS;
void CAN2_RX1_IRQHandler(void) WEAK_ALIAS;
void CAN2_SCE_IRQHandler(void) WEAK_ALIAS;
void OTG_FS_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream5_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream6_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream7_IRQHandler(void) WEAK_ALIAS;
void USART6_IRQHandler(void) WEAK_ALIAS;
void I2C3_EV_IRQHandler(void) WEAK_ALIAS;
void I2C3_ER_IRQHandler(void) WEAK_ALIAS;
void OTG_HS_EP1_OUT_IRQHandler(void) WEAK_ALIAS;
void OTG_HS_EP1_IN_IRQHandler(void) WEAK_ALIAS;
void OTG_HS_WKUP_IRQHandler(void) WEAK_ALIAS;
void OTG_HS_IRQHandler(void) WEAK_ALIAS;
void DCMI_IRQHandler(void) WEAK_ALIAS;
void FPU_IRQHandler(void) WEAK_ALIAS;
void SPI4_IRQHandler(void) WEAK_ALIAS;
void SAI1_IRQHandler(void) WEAK_ALIAS;
void SAI2_IRQHandler(void) WEAK_ALIAS;
void QUADSPI_IRQHandler(void) WEAK_ALIAS;
void CEC_IRQHandler(void) WEAK_ALIAS;
void SPDIF_RX_IRQHandler(void) WEAK_ALIAS;
void FMPI2C1_Event_IRQHandler(void) WEAK_ALIAS;
void FMPI2C1_Error_IRQHandler(void) WEAK_ALIAS;

/*-----------------------------------------------------------------------*/
/* Vector table                                                          */
/*                                                                       */
/* Entry 0 is not a function pointer at all -- it's the initial stack    */
/* pointer value. ISO C does not define object-pointer -> function-      */
/* pointer conversion (that's what -Wpedantic was correctly flagging),   */
/* so instead of casting &_estack to a function pointer type, each       */
/* array element is a union that can legally hold either a handler       */
/* pointer or a plain address. This is the same pattern CMSIS itself     */
/* uses in its C-based startup files.                                    */
/*-----------------------------------------------------------------------*/
typedef union
{
    void (*handler)(void);
    uint32_t stackptr;
} VectorEntry;

__attribute__((section(".isr_vector"), used)) const VectorEntry g_pfnVectors[] = {
    {.stackptr = (uint32_t)&_estack}, /* 0  Initial Stack Pointer */
    {.handler = Reset_Handler},       /* 1  Reset                 */
    {.handler = NMI_Handler},         /* 2  NMI                   */
    {.handler = HardFault_Handler},   /* 3  Hard Fault            */
    {.handler = MemManage_Handler},   /* 4  MPU Fault             */
    {.handler = BusFault_Handler},    /* 5  Bus Fault             */
    {.handler = UsageFault_Handler},  /* 6  Usage Fault           */
    {.stackptr = 0},                  /* 7  Reserved              */
    {.stackptr = 0},                  /* 8  Reserved              */
    {.stackptr = 0},                  /* 9  Reserved              */
    {.stackptr = 0},                  /* 10 Reserved              */
    {.handler = SVC_Handler},         /* 11 SVCall                */
    {.handler = DebugMon_Handler},    /* 12 Debug Monitor         */
    {.stackptr = 0},                  /* 13 Reserved              */
    {.handler = PendSV_Handler},      /* 14 PendSV                */
    {.handler = SysTick_Handler},     /* 15 SysTick               */

    /* ---- External interrupts (IRQ0..IRQ96), positions 16..112 ---- */
    {.handler = WWDG_IRQHandler},               /* 16  IRQ0  WWDG */
    {.handler = PVD_IRQHandler},                /* 17  IRQ1  PVD through EXTI */
    {.handler = TAMP_STAMP_IRQHandler},         /* 18  IRQ2  Tamper/TimeStamp */
    {.handler = RTC_WKUP_IRQHandler},           /* 19  IRQ3  RTC Wakeup */
    {.handler = FLASH_IRQHandler},              /* 20  IRQ4  Flash */
    {.handler = RCC_IRQHandler},                /* 21  IRQ5  RCC */
    {.handler = EXTI0_IRQHandler},              /* 22  IRQ6  EXTI0 */
    {.handler = EXTI1_IRQHandler},              /* 23  IRQ7  EXTI1 */
    {.handler = EXTI2_IRQHandler},              /* 24  IRQ8  EXTI2 */
    {.handler = EXTI3_IRQHandler},              /* 25  IRQ9  EXTI3 */
    {.handler = EXTI4_IRQHandler},              /* 26  IRQ10 EXTI4 */
    {.handler = DMA1_Stream0_IRQHandler},       /* 27  IRQ11 */
    {.handler = DMA1_Stream1_IRQHandler},       /* 28  IRQ12 */
    {.handler = DMA1_Stream2_IRQHandler},       /* 29  IRQ13 */
    {.handler = DMA1_Stream3_IRQHandler},       /* 30  IRQ14 */
    {.handler = DMA1_Stream4_IRQHandler},       /* 31  IRQ15 */
    {.handler = DMA1_Stream5_IRQHandler},       /* 32  IRQ16 */
    {.handler = DMA1_Stream6_IRQHandler},       /* 33  IRQ17 */
    {.handler = ADC_IRQHandler},                /* 34  IRQ18 ADC1/2/3 */
    {.handler = CAN1_TX_IRQHandler},            /* 35  IRQ19 */
    {.handler = CAN1_RX0_IRQHandler},           /* 36  IRQ20 */
    {.handler = CAN1_RX1_IRQHandler},           /* 37  IRQ21 */
    {.handler = CAN1_SCE_IRQHandler},           /* 38  IRQ22 */
    {.handler = EXTI9_5_IRQHandler},            /* 39  IRQ23 */
    {.handler = TIM1_BRK_TIM9_IRQHandler},      /* 40  IRQ24 */
    {.handler = TIM1_UP_TIM10_IRQHandler},      /* 41  IRQ25 */
    {.handler = TIM1_TRG_COM_TIM11_IRQHandler}, /* 42  IRQ26 */
    {.handler = TIM1_CC_IRQHandler},            /* 43  IRQ27 */
    {.handler = TIM2_IRQHandler},               /* 44  IRQ28 */
    {.handler = TIM3_IRQHandler},               /* 45  IRQ29 */
    {.handler = TIM4_IRQHandler},               /* 46  IRQ30 */
    {.handler = I2C1_EV_IRQHandler},            /* 47  IRQ31 */
    {.handler = I2C1_ER_IRQHandler},            /* 48  IRQ32 */
    {.handler = I2C2_EV_IRQHandler},            /* 49  IRQ33 */
    {.handler = I2C2_ER_IRQHandler},            /* 50  IRQ34 */
    {.handler = SPI1_IRQHandler},               /* 51  IRQ35 */
    {.handler = SPI2_IRQHandler},               /* 52  IRQ36 */
    {.handler = USART1_IRQHandler},             /* 53  IRQ37 */
    {.handler = USART2_IRQHandler},             /* 54  IRQ38 */
    {.handler = USART3_IRQHandler},             /* 55  IRQ39 */
    {.handler = EXTI15_10_IRQHandler},          /* 56  IRQ40 */
    {.handler = RTC_Alarm_IRQHandler},          /* 57  IRQ41 */
    {.handler = OTG_FS_WKUP_IRQHandler},        /* 58  IRQ42 */
    {.handler = TIM8_BRK_TIM12_IRQHandler},     /* 59  IRQ43 */
    {.handler = TIM8_UP_TIM13_IRQHandler},      /* 60  IRQ44 */
    {.handler = TIM8_TRG_COM_TIM14_IRQHandler}, /* 61  IRQ45 */
    {.handler = TIM8_CC_IRQHandler},            /* 62  IRQ46 */
    {.handler = DMA1_Stream7_IRQHandler},       /* 63  IRQ47 */
    {.handler = FMC_IRQHandler},                /* 64  IRQ48 */
    {.handler = SDIO_IRQHandler},               /* 65  IRQ49 */
    {.handler = TIM5_IRQHandler},               /* 66  IRQ50 */
    {.handler = SPI3_IRQHandler},               /* 67  IRQ51 */
    {.handler = UART4_IRQHandler},              /* 68  IRQ52 */
    {.handler = UART5_IRQHandler},              /* 69  IRQ53 */
    {.handler = TIM6_DAC_IRQHandler},           /* 70  IRQ54 */
    {.handler = TIM7_IRQHandler},               /* 71  IRQ55 */
    {.handler = DMA2_Stream0_IRQHandler},       /* 72  IRQ56 */
    {.handler = DMA2_Stream1_IRQHandler},       /* 73  IRQ57 */
    {.handler = DMA2_Stream2_IRQHandler},       /* 74  IRQ58 */
    {.handler = DMA2_Stream3_IRQHandler},       /* 75  IRQ59 */
    {.handler = DMA2_Stream4_IRQHandler},       /* 76  IRQ60 */
    {.stackptr = 0},                            /* 77  IRQ61 Reserved */
    {.stackptr = 0},                            /* 78  IRQ62 Reserved */
    {.handler = CAN2_TX_IRQHandler},            /* 79  IRQ63 */
    {.handler = CAN2_RX0_IRQHandler},           /* 80  IRQ64 */
    {.handler = CAN2_RX1_IRQHandler},           /* 81  IRQ65 */
    {.handler = CAN2_SCE_IRQHandler},           /* 82  IRQ66 */
    {.handler = OTG_FS_IRQHandler},             /* 83  IRQ67 */
    {.handler = DMA2_Stream5_IRQHandler},       /* 84  IRQ68 */
    {.handler = DMA2_Stream6_IRQHandler},       /* 85  IRQ69 */
    {.handler = DMA2_Stream7_IRQHandler},       /* 86  IRQ70 */
    {.handler = USART6_IRQHandler},             /* 87  IRQ71 */
    {.handler = I2C3_EV_IRQHandler},            /* 88  IRQ72 */
    {.handler = I2C3_ER_IRQHandler},            /* 89  IRQ73 */
    {.handler = OTG_HS_EP1_OUT_IRQHandler},     /* 90  IRQ74 */
    {.handler = OTG_HS_EP1_IN_IRQHandler},      /* 91  IRQ75 */
    {.handler = OTG_HS_WKUP_IRQHandler},        /* 92  IRQ76 */
    {.handler = OTG_HS_IRQHandler},             /* 93  IRQ77 */
    {.handler = DCMI_IRQHandler},               /* 94  IRQ78 */
    {.stackptr = 0},                            /* 95  IRQ79 Reserved */
    {.stackptr = 0},                            /* 96  IRQ80 Reserved */
    {.handler = FPU_IRQHandler},                /* 97  IRQ81 */
    {.stackptr = 0},                            /* 98  IRQ82 Reserved */
    {.stackptr = 0},                            /* 99  IRQ83 Reserved */
    {.handler = SPI4_IRQHandler},               /* 100 IRQ84 */
    {.stackptr = 0},                            /* 101 IRQ85 Reserved */
    {.stackptr = 0},                            /* 102 IRQ86 Reserved */
    {.handler = SAI1_IRQHandler},               /* 103 IRQ87 */
    {.stackptr = 0},                            /* 104 IRQ88 Reserved */
    {.stackptr = 0},                            /* 105 IRQ89 Reserved */
    {.stackptr = 0},                            /* 106 IRQ90 Reserved */
    {.handler = SAI2_IRQHandler},               /* 107 IRQ91 */
    {.handler = QUADSPI_IRQHandler},            /* 108 IRQ92 */
    {.handler = CEC_IRQHandler},                /* 109 IRQ93 */
    {.handler = SPDIF_RX_IRQHandler},           /* 110 IRQ94 */
    {.handler = FMPI2C1_Event_IRQHandler},      /* 111 IRQ95 */
    {.handler = FMPI2C1_Error_IRQHandler},      /* 112 IRQ96 */
};

/*-----------------------------------------------------------------------*/
/* Reset_Handler                                                         */
/*-----------------------------------------------------------------------*/
void Reset_Handler(void)
{
    /* Clock tree / early hardware init (CMSIS system_stm32f4xx.c) */
    SystemInit();

    uint32_t *src, *dst;

    /* Copy .data initial values from Flash (LMA) to SRAM (VMA) */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zero-fill .bss in SRAM */
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }


    /* Hand off to the application */
    (void)main();

    /* main() must never return in a bare-metal image. If it does
     * anyway (compiler bug, bad linker script, corrupted stack...),
     * spin here rather than falling off into whatever code or data
     * happens to sit past this function. */
    while (1)
    {
    }
}
