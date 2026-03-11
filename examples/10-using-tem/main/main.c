#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "wifi_conn.h"
#include "dht11.h"
#include "button.h"
#include "coze_api.h"

static const char *TAG = "MAIN";

static void main_task(void *pvParameters) {
    QueueHandle_t button_queue = button_get_event_queue();
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(button_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(50));          // 消抖
            if (gpio_get_level(io_num) == 0) {
                if (!wifi_is_connected()) {
                    ESP_LOGW(TAG, "WiFi未连接，跳过请求");
                    continue;
                }

                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
                    ESP_LOGW(TAG, "WiFi未关联到热点");
                    continue;
                }

                float temp, humid;
                int retries = 3;
                while (retries--) {
                    if (dht11_read(&temp, &humid) == 0) {
                        ESP_LOGI(TAG, "DHT11读取成功: 温度=%.1f℃, 湿度=%.1f%%", temp, humid);
                        coze_send_request(temp, humid);
                        break;
                    } else {
                        ESP_LOGE(TAG, "DHT11读取失败，剩余重试次数: %d", retries);
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    }
                }
                if (retries < 0) {
                    ESP_LOGE(TAG, "DHT11多次读取失败！");
                }
            }
        }
    }
}

void app_main(void) {
    // 设置日志级别
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_DEBUG);
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_DEBUG);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("tls_transport", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "系统启动中...");

    wifi_conn_init();
    button_init();
    dht11_init();

    xTaskCreate(main_task, "main_task", 32768, NULL, 5, NULL);
}