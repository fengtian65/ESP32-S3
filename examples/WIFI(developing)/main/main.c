#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

// 手机热点配置信息
#define WIFI_SSID "Your_Hotspot_Name"  // 替换为你的手机热点名称
#define WIFI_PASS "Your_Hotspot_Password"  // 替换为你的手机热点密码

// Wi-Fi事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        printf("WiFi connected successfully!\n");
    }
}

void app_main(void)
{
    // 初始化NVS
    nvs_flash_init();
    
    // 初始化事件循环
    esp_event_loop_create_default();
    
    // 配置Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // 注册事件处理函数（只关注IP获取事件）
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, 
        [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
            esp_wifi_connect();
        }, NULL);
    
    // 配置为Station模式并设置SSID和密码
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    // 启动Wi-Fi
    esp_wifi_start();
}