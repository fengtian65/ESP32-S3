#include <stdio.h>
#include "driver/gpio.h"

void app_main(void)
{
    gpio_config_t io_conf={
        .pin_bit_mask = (1ULL<<15),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLUP_DISABLE,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_15, 1);
}
