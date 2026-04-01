#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_conn.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 WiFi 功能扩展教学例程启动");
    
    wifi_conn_init();
    
    uint8_t demo_step = 0;
    
    while (1) {
        if (wifi_is_connected()) {
            char ip[16];
            char ssid[33];
            int8_t rssi = wifi_get_rssi();
            wifi_get_ip(ip, sizeof(ip));
            wifi_get_ssid(ssid, sizeof(ssid));
            
            ESP_LOGI(TAG, "已连接 - SSID: %s, IP: %s, RSSI: %d dBm", ssid, ip, rssi);
        } else {
            ESP_LOGI(TAG, "WiFi 未连接");
        }
        
        if (demo_step == 5 && wifi_is_connected()) {
            ESP_LOGI(TAG, "演示: 扫描附近的 WiFi...");
            wifi_ap_info_t ap_list[MAX_SCAN_RESULTS];
            uint16_t ap_count = MAX_SCAN_RESULTS;
            
            if (wifi_scan_ap(ap_list, &ap_count) == ESP_OK) {
                ESP_LOGI(TAG, "扫描完成，发现 %d 个 AP:", ap_count);
                for (uint16_t i = 0; i < ap_count; i++) {
                    ESP_LOGI(TAG, "  [%d] %s (RSSI: %d dBm)", i + 1, ap_list[i].ssid, ap_list[i].rssi);
                }
            }
            demo_step++;
        }
        
        demo_step++;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
