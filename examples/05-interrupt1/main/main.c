#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

volatile bool gpio_interrupt_flag = false;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    printf("基础中断触发！（GPIO%d）\n", GPIO_NUM_11);
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

void app_main(void)
{
    gpio_init();
}