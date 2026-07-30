#include "common.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "gpio.h"
#include "input_capture.h"
#include "pwm.h"
#include "task.h"
#include "uart.h"
#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)

#include <stdint.h>
#include <stdio.h>
TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;
void vTask1(void *pvParameters)
{
    for (;;)
    {

        UNUSED(pvParameters);
        GPIO_TogglePin(GPIOA, 5);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTask2(void *pvParameters)
{
    for (;;)
    {
        printf("Task 2 (Low Priority) running\r\n");
        UNUSED(pvParameters);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskENTER_CRITICAL();
    UNUSED(xTask);
    printf("stack overflow, task: %s", pcTaskName);

    while (1)
    {
    }
}

int main(void)
{
    uart_init(115200);

    GPIO_Config ld2_cfg = {
        .mode      = GPIO_MODE_OUTPUT,
        .otype     = GPIO_OTYPE_PUSH_PULL,
        .speed     = GPIO_SPEED_LOW,
        .pupd      = GPIO_PUPD_NONE,
        .alternate = 0 // unused, not in alternate mode
    };
    GPIO_Init(GPIOA, 5, ld2_cfg);

    BaseType_t xReturned;

    /* Create Task 1 with higher priority */
    xReturned = xTaskCreate(vTask1, "Task1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2,
                            &xTaskHandle1);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    /* Create Task 2 with lower priority */
    xReturned = xTaskCreate(vTask2, "Task2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1,
                            &xTaskHandle2);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    vTaskStartScheduler();

    /* Should never reach here */
    for (;;)
        ;

    return 0;
}
