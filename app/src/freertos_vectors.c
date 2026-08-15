extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

void __attribute__((naked)) SVC_Handler(void)
{
    __asm volatile("b vPortSVCHandler");
}

void __attribute__((naked)) PendSV_Handler(void)
{
    __asm volatile("b xPortPendSVHandler");
}

void SysTick_Handler(void)
{
    xPortSysTickHandler();
}
