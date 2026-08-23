#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
typedef long BaseType_t;
typedef void *SemaphoreHandle_t;
#define pdFALSE 0
#define pdTRUE  1
extern SemaphoreHandle_t mock_last_semaphore_given;
extern int mock_semaphore_give_call_count;
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << (8 - 4))
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,
                                 BaseType_t *pxHigherPriorityTaskWoken);
void portYIELD_FROM_ISR(BaseType_t x);

#endif
