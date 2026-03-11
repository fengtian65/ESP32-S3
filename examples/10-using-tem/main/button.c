#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"

#define BUTTON_GPIO GPIO_NUM_11
static const char *TAG = "BUTTON";
static QueueHandle_t g_button_evt_queue = NULL;

static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(g_button_evt_queue, &gpio_num, NULL);
}

void button_init(void) {
    g_button_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void*) BUTTON_GPIO);
    ESP_LOGI(TAG, "Button initialized on GPIO%d", BUTTON_GPIO);
}

QueueHandle_t button_get_event_queue(void) {
    return g_button_evt_queue;
}