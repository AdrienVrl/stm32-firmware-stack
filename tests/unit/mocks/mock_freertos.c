#include "mock_freertos.h"

#include <stddef.h>

SemaphoreHandle_t mock_last_semaphore_given = NULL;
int mock_semaphore_give_call_count          = 0;

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,
                                 BaseType_t *pxHigherPriorityTaskWoken)
{
    mock_last_semaphore_given = xSemaphore;
    mock_semaphore_give_call_count++;
    *pxHigherPriorityTaskWoken = pdTRUE;
    return pdTRUE;
}

void portYIELD_FROM_ISR(BaseType_t x)
{
    (void)x;
}
