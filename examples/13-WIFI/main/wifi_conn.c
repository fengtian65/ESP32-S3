#include "wifi_conn.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <string.h>

static const char *TAG = "WIFI_CONN";
static bool g_wifi_connected = false;
static esp_ip4_addr_t g_ip_addr = {0};
static char g_current_ssid[33] = {0};
static bool g_auto_reconnect = true;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_connected = false;
        ESP_LOGW(TAG, "Disconnected");
        if (g_auto_reconnect) {
            ESP_LOGI(TAG, "Auto reconnect enabled, reconnecting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_wifi_connected = true;
        ip_event_got_ip_t* ip_event = (ip_event_got_ip_t*)event_data;
        g_ip_addr = ip_event->ip_info.ip;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&ip_event->ip_info.ip));
    }
}

void wifi_conn_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -70,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy(g_current_ssid, WIFI_SSID, sizeof(g_current_ssid) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized, connecting to %s", WIFI_SSID);
}

bool wifi_is_connected(void) {
    return g_wifi_connected;
}

void wifi_get_ip(char *ip_buf, size_t buf_size) {
    if (ip_buf == NULL || buf_size < 16) {
        return;
    }
    snprintf(ip_buf, buf_size, IPSTR, IP2STR(&g_ip_addr));
}

int8_t wifi_get_rssi(void) {
    if (!g_wifi_connected) {
        return -127;
    }
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        return ap_info.rssi;
    }
    return -127;
}

esp_err_t wifi_scan_ap(wifi_ap_info_t *ap_list, uint16_t *ap_count) {
    if (ap_list == NULL || ap_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t max_count = *ap_count;
    if (max_count > MAX_SCAN_RESULTS) {
        max_count = MAX_SCAN_RESULTS;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    ESP_LOGI(TAG, "Starting WiFi scan...");
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    wifi_ap_record_t ap_records[MAX_SCAN_RESULTS];
    uint16_t found = MAX_SCAN_RESULTS;
    ret = esp_wifi_scan_get_ap_records(&found, ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Get scan records failed: %s", esp_err_to_name(ret));
        return ret;
    }

    uint16_t copy_count = (found < max_count) ? found : max_count;
    for (uint16_t i = 0; i < copy_count; i++) {
        strncpy(ap_list[i].ssid, (char*)ap_records[i].ssid, sizeof(ap_list[i].ssid) - 1);
        ap_list[i].ssid[sizeof(ap_list[i].ssid) - 1] = '\0';
        ap_list[i].rssi = ap_records[i].rssi;
        ap_list[i].authmode = ap_records[i].authmode;
    }
    *ap_count = copy_count;

    ESP_LOGI(TAG, "Scan complete, found %d APs", found);
    return ESP_OK;
}

esp_err_t wifi_connect_to_ap(const char *ssid, const char *password) {
    if (ssid == NULL || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    g_auto_reconnect = true;
    strncpy(g_current_ssid, ssid, sizeof(g_current_ssid) - 1);

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    wifi_config.sta.threshold.rssi = -70;
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "Connecting to %s...", ssid);
    return esp_wifi_connect();
}

void wifi_disconnect(void) {
    g_auto_reconnect = false;
    esp_wifi_disconnect();
    g_wifi_connected = false;
    ESP_LOGI(TAG, "WiFi disconnected");
}

void wifi_reconnect(void) {
    g_auto_reconnect = true;
    if (g_wifi_connected) {
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    esp_wifi_connect();
    ESP_LOGI(TAG, "Reconnecting to %s...", g_current_ssid);
}

void wifi_get_ssid(char *ssid_buf, size_t buf_size) {
    if (ssid_buf == NULL || buf_size < 1) {
        return;
    }
    strncpy(ssid_buf, g_current_ssid, buf_size - 1);
    ssid_buf[buf_size - 1] = '\0';
}
