#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void button_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_11),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_config);
}

void LED_init()
{
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_15)|(1ULL << GPIO_NUM_16)|(1ULL << GPIO_NUM_17),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_config);
}

void button_task(void *pvParameters)
{
    int a = 0;
    while (1)
    {
        if (gpio_get_level(GPIO_NUM_11) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            if(gpio_get_level(GPIO_NUM_11) == 1)
            {
                continue;
            }
            while (gpio_get_level(GPIO_NUM_11) == 0)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            a++;
        }
        if (a>2)
        {
            a = 0;
        }
        if (a == 0)
        {
            gpio_set_level(GPIO_NUM_15, 1);
            gpio_set_level(GPIO_NUM_16, 0);
            gpio_set_level(GPIO_NUM_17, 0);
        }
        else if (a == 1)
        {
            gpio_set_level(GPIO_NUM_15, 0);
            gpio_set_level(GPIO_NUM_16, 1);
            gpio_set_level(GPIO_NUM_17, 0);
        }
        else if (a == 2)
        {
            gpio_set_level(GPIO_NUM_15, 0);
            gpio_set_level(GPIO_NUM_16, 0);
            gpio_set_level(GPIO_NUM_17, 1);
        }
    }
}

void app_main(void)
{
    button_init();
    LED_init();
    xTaskCreate(
        button_task, 
        "button_task",
        2048, 
        NULL,
        5, 
        NULL);
}
