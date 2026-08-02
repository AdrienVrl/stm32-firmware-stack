#include "FreeRTOS.h"

#include "event_groups.h"
#include "FreeRTOSConfig.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "common.h"
#include "exti.h"
#include "gpio.h"
#include "i2c.h"
#include "input_capture.h"
#include "pwm.h"
#include "uart.h"

#include <stdint.h>
#include <stdio.h>
#define GPIOA_BASE 0x40020000UL
#define GPIOA      ((GPIO_Port *)GPIOA_BASE)
#define GPIOC_BASE 0x40020800UL
#define GPIOC      ((GPIO_Port *)GPIOC_BASE)

TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;
TaskHandle_t xTaskHandle3 = NULL;
TaskHandle_t xTaskHandle4 = NULL;
TaskHandle_t xTaskHandle5 = NULL;
TaskHandle_t xTaskHandle6 = NULL;

QueueHandle_t xSensorQueue, xProcessorQueue;
uint32_t sensor_drop_count_snd;
uint32_t sensor_drop_count_rcv;
uint32_t processed_drop_count_snd;
uint32_t processed_drop_count_rcv;

#define WDG_BIT_SENSOR_READER (1 << 0)
#define WDG_BIT_PROCESSOR     (1 << 1)
#define WDG_BIT_OUTPUT        (1 << 2)
#define WDG_BIT_HEARTBEAT     (1 << 3)

#define WDG_ALL_TASKS_BITS                                                                         \
    (WDG_BIT_SENSOR_READER | WDG_BIT_PROCESSOR | WDG_BIT_OUTPUT | WDG_BIT_HEARTBEAT)

EventGroupHandle_t xWatchdogEvents;

typedef struct
{
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
    uint16_t gyro_x;
    uint16_t gyro_y;
    uint16_t gyro_z;
} SensorData;

typedef struct
{
    float accel_g[3];
    float gyro_dps[3];
} ProcessedData;

uint16_t merge(uint8_t low, uint8_t high)
{
    return (low | (high << 8));
}

void vSensorReaderTask(void *pvParameters)
{
    UNUSED(pvParameters);
    for (;;)
    {
        uint8_t accel_x_1 = i2c_read_reg(0x68, 0x3C);
        uint8_t accel_x_2 = i2c_read_reg(0x68, 0x3B);
        uint8_t accel_y_1 = i2c_read_reg(0x68, 0x3E);
        uint8_t accel_y_2 = i2c_read_reg(0x68, 0x3D);
        uint8_t accel_z_1 = i2c_read_reg(0x68, 0x40);
        uint8_t accel_z_2 = i2c_read_reg(0x68, 0x3F);

        uint8_t gyro_x_1 = i2c_read_reg(0x68, 0x44);
        uint8_t gyro_x_2 = i2c_read_reg(0x68, 0x43);
        uint8_t gyro_y_1 = i2c_read_reg(0x68, 0x46);
        uint8_t gyro_y_2 = i2c_read_reg(0x68, 0x45);
        uint8_t gyro_z_1 = i2c_read_reg(0x68, 0x48);
        uint8_t gyro_z_2 = i2c_read_reg(0x68, 0x47);

        SensorData data = {merge(accel_x_1, accel_x_2), merge(accel_y_1, accel_y_2),
                           merge(accel_z_1, accel_z_2), merge(gyro_x_1, gyro_x_2),
                           merge(gyro_y_1, gyro_y_2),   merge(gyro_z_1, gyro_z_2)};

        if (xSensorQueue != 0)
        {
            if (xQueueSend(xSensorQueue, (void *)&data, pdMS_TO_TICKS(10)))
            {
            }
            else
            {
                sensor_drop_count_snd++;
            }
        }
        xEventGroupSetBits(xWatchdogEvents, WDG_BIT_SENSOR_READER);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vSensorProcessorTask(void *pvParameters)
{

    UNUSED(pvParameters);
    for (;;)
    {
        SensorData xData;

        if (xSensorQueue != NULL)
        {
            if (xQueueReceive(xSensorQueue, &(xData), pdMS_TO_TICKS(10)))
            {
            }
            else
            {
                sensor_drop_count_rcv++;
            }
        }

        ProcessedData xProcessedData = {
            {xData.accel_x / 16384.0f, xData.accel_y / 16384.0f, xData.accel_z / 16384.0f},
            {xData.gyro_x / 131.0f, xData.gyro_y / 131.0f, xData.gyro_z / 131.0f}};

        if (xProcessorQueue != 0)
        {
            if (xQueueSend(xProcessorQueue, (void *)&xProcessedData, pdMS_TO_TICKS(10)))
            {
            }
            else
            {
                processed_drop_count_snd++;
            }
        }

        xEventGroupSetBits(xWatchdogEvents, WDG_BIT_PROCESSOR);
    }
}

void vSensorOutputTask(void *pvParameters)
{
    UNUSED(pvParameters);
    for (;;)
    {
        ProcessedData xData;

        if (xProcessorQueue != NULL)
        {
            if (xQueueReceive(xProcessorQueue, &(xData), pdMS_TO_TICKS(10)))
            {
            }
            else
            {
                processed_drop_count_rcv++;
            }
        }

        printf("Accel_x: %f, Accel_y: %f, Accel_z: %f, Gyro_x: %f, Gyro_y: %f, "
               "Gyro_z: %f\r\n",
               xData.accel_g[0], xData.accel_g[1], xData.accel_g[2], xData.gyro_dps[0],
               xData.gyro_dps[1], xData.gyro_dps[2]);

        xEventGroupSetBits(xWatchdogEvents, WDG_BIT_OUTPUT);
    }
}

void vHeartBeatTask(void *pvParameters)
{

    UNUSED(pvParameters);

    for (;;)
    {
        printf("HWM Reader=%lu Processor=%lu Output=%lu HeartBeat=%lu\r\n",
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskHandle1),
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskHandle2),
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskHandle3),
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskHandle4));
        GPIO_TogglePin(GPIOA, 5);

        xEventGroupSetBits(xWatchdogEvents, WDG_BIT_HEARTBEAT);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

SemaphoreHandle_t xButtonSemaphore;

void vButtonTask(void *pvParameters)
{
    UNUSED(pvParameters);
    configASSERT(xButtonSemaphore != NULL);

    for (;;)
    {
        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("Button pressed\r\n");
        }
    }
}

void vWatchdogTask(void *pvParameters)
{

    UNUSED(pvParameters);
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        EventBits_t bits_before_reset = xEventGroupClearBits(xWatchdogEvents, WDG_ALL_TASKS_BITS);
        EventBits_t missing           = (~bits_before_reset) & WDG_ALL_TASKS_BITS;

        if (missing != 0)
        {
            if (missing & WDG_BIT_SENSOR_READER)
                printf("WDG FAULT: sensor reader missed check-in\r\n");
            if (missing & WDG_BIT_PROCESSOR)
                printf("WDG FAULT: processor missed check-in\r\n");
            if (missing & WDG_BIT_OUTPUT)
                printf("WDG FAULT: output missed check-in\r\n");
            if (missing & WDG_BIT_HEARTBEAT)
                printf("WDG FAULT: heartbeat missed check-in\r\n");
        }
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    UNUSED(xTask);
    printf("stack overflow, task: %s", pcTaskName);
    portDISABLE_INTERRUPTS();
    while (1)
    {
    }
}

int main(void)
{
    uart_init(115200);
    i2c_init(1000);
    i2c_write_reg(0x68, 0x6B, 0x00);

    GPIO_Config ld2_cfg = {
        .mode      = GPIO_MODE_OUTPUT,
        .otype     = GPIO_OTYPE_PUSH_PULL,
        .speed     = GPIO_SPEED_LOW,
        .pupd      = GPIO_PUPD_NONE,
        .alternate = 0 // unused, not in alternate mode
    };
    GPIO_Init(GPIOA, 5, ld2_cfg);

    xButtonSemaphore = xSemaphoreCreateBinary();
    ButtonEXTI_Init();

    BaseType_t xReturned;

    xSensorQueue    = xQueueCreate(10, sizeof(SensorData));
    xProcessorQueue = xQueueCreate(10, sizeof(ProcessedData));

    xWatchdogEvents = xEventGroupCreate();
    configASSERT(xWatchdogEvents != NULL);

    sensor_drop_count_snd    = 0;
    sensor_drop_count_rcv    = 0;
    processed_drop_count_snd = 0;
    processed_drop_count_rcv = 0;

    xReturned = xTaskCreate(vSensorReaderTask, "SensorReader", configMINIMAL_STACK_SIZE, NULL,
                            tskIDLE_PRIORITY + 4, &xTaskHandle1);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    xReturned = xTaskCreate(vSensorProcessorTask, "SensorProcessor", configMINIMAL_STACK_SIZE, NULL,
                            tskIDLE_PRIORITY + 3, &xTaskHandle2);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    xReturned = xTaskCreate(vSensorOutputTask, "SensorOutput", configMINIMAL_STACK_SIZE * 3, NULL,
                            tskIDLE_PRIORITY + 1, &xTaskHandle3);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    xReturned = xTaskCreate(vHeartBeatTask, "HeartBeat", configMINIMAL_STACK_SIZE + 64, NULL,
                            tskIDLE_PRIORITY + 2, &xTaskHandle4);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    xReturned = xTaskCreate(vButtonTask, "Button", configMINIMAL_STACK_SIZE * 4, NULL,
                            tskIDLE_PRIORITY + 1, &xTaskHandle5);

    if (xReturned != pdPASS)
    {
        for (;;)
            ;
    }

    xReturned = xTaskCreate(vWatchdogTask, "Watchdog", configMINIMAL_STACK_SIZE * 4, NULL,
                            tskIDLE_PRIORITY + 4, &xTaskHandle6);

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
