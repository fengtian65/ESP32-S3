#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

/* ==================== 静态变量 ==================== */
static const char *TAG = "DHT11";
static SemaphoreHandle_t g_dht11_mutex = NULL;
#define DHT11_SAMPLE_US 25

/* ==================== 内部静态函数 ==================== */
__attribute__((optimize("O0")))
static inline void dht11_delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}

/* ==================== 公开函数实现 ==================== */
void dht11_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DHT11_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    g_dht11_mutex = xSemaphoreCreateMutex();
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "DHT11 initialized on GPIO%d", DHT11_GPIO);
}

int dht11_read(float *temp, float *humid) {
    uint8_t data[5] = {0};
    int ret = -1;
    uint8_t raw_data[5] = {0};
    uint8_t checksum = 0;
    int read_stage = 0;

    if (xSemaphoreTake(g_dht11_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        ESP_LOGE(TAG, "Mutex timeout");
        return -1;
    }

    // 总线重置
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    dht11_delay_us(500);

    // 主机起始信号
    gpio_set_level(DHT11_GPIO, 0);
    dht11_delay_us(20000);
    gpio_set_level(DHT11_GPIO, 1);
    dht11_delay_us(30);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);

    // 等待响应
    read_stage = 1;
    uint32_t timeout = 2000;
    while (gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
    if (timeout == 0) { goto exit; }

    timeout = 100;
    while (gpio_get_level(DHT11_GPIO) == 0 && timeout--) dht11_delay_us(1);
    if (timeout == 0) { goto exit; }

    timeout = 100;
    while (gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
    if (timeout == 0) { goto exit; }

    // 读取40bit数据
    read_stage = 2;
    for (int i = 0; i < 40; i++) {
        timeout = 100;
        while (gpio_get_level(DHT11_GPIO) == 0 && timeout--) dht11_delay_us(1);
        if (timeout == 0) { goto exit; }

        dht11_delay_us(DHT11_SAMPLE_US);
        data[i/8] <<= 1;
        if (gpio_get_level(DHT11_GPIO) == 1) {
            data[i/8] |= 1;
        }

        timeout = 200;
        while (gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
        if (timeout == 0) { goto exit; }
    }

    memcpy(raw_data, data, 5);

    // 校验和验证
    read_stage = 3;
    checksum = data[0] + data[1] + data[2] + data[3];
    if (abs(checksum - data[4]) <= 1) {
        *humid = (float)data[0];
        *temp = (float)data[2];
        ret = 0;
    }

exit:
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);
    
    xSemaphoreGive(g_dht11_mutex);

    ESP_LOGI(TAG, "Raw data: %02x %02x %02x %02x %02x", raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4]);
    if (ret != 0) {
        if (read_stage < 3) {
            ESP_LOGE(TAG, "Read timeout at stage %d", read_stage);
        } else {
            ESP_LOGE(TAG, "Checksum failed (calc: %02x, read: %02x)", checksum, raw_data[4]);
        }
    } else {
        ESP_LOGI(TAG, "Read success: Temp=%.1f℃, Humid=%.1f%%", *temp, *humid);
    }
    return ret;
}