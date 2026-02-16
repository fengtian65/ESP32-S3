#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<15)|(1ULL<<16)|(1ULL<<17),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    int a = 0;
    while (1)
    {
        if (a > 2)
        {
            a = 0;
        }
        if (a == 0)
        {
            gpio_set_level(15, 1);
            gpio_set_level(16, 0);
            gpio_set_level(17, 0);
        }
        else if (a == 1)
        {
            gpio_set_level(15, 0);
            gpio_set_level(16, 1);
            gpio_set_level(17, 0);
        }
        else if (a == 2)
        {
            gpio_set_level(15, 0);
            gpio_set_level(16, 0);
            gpio_set_level(17, 1);
        }
        a++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
