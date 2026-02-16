#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

SemaphoreHandle_t gpio_interrupt_sem = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // 给信号量发信号，通知任务处理（ISR版本的信号量API）
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(gpio_interrupt_sem, &xHigherPriorityTaskWoken);
    
    // 如果需要切换任务，触发上下文切换
    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void gpio_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_11),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_config);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_11, gpio_isr_handler, NULL);
}

void interrupt_process_task(void *arg)
{
    while(1) {
        // 等待信号量（阻塞直到中断触发）
        if(xSemaphoreTake(gpio_interrupt_sem, portMAX_DELAY) == pdTRUE) {
            // 安全打印中断触发信息
            printf("GPIO interrupt triggered! (GPIO%d)\n", GPIO_NUM_11);
            
            // 可选：添加短延时，避免按钮抖动导致多次触发
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void app_main(void)
{
    gpio_interrupt_sem = xSemaphoreCreateBinary();
    gpio_init();
    xTaskCreate(
        interrupt_process_task,   // 任务函数
        "interrupt_process",      // 任务名
        2048,                     // 栈大小
        NULL,                     // 任务参数
        1,                        // 优先级
        NULL                      // 任务句柄
    );
}