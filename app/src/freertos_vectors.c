extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

void SVC_Handler(void)
{
    vPortSVCHandler();
}
void PendSV_Handler(void)
{
    xPortPendSVHandler();
}
void SysTick_Handler(void)
{
    xPortSysTickHandler();
}
