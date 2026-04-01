#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mpu6050.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#define TAG "MAIN"

#define WIFI_SSID "zhangfanding"
#define WIFI_PASS "12345678"
#define SERVER_IP "192.168.252.149"
#define SERVER_PORT 8888

#define WIFI_CONNECTED_BIT BIT0
#define TCP_CONNECTED_BIT BIT1

static int sock = -1;
static EventGroupHandle_t s_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT | TCP_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static int tcp_connect(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        sock = -1;
        xEventGroupClearBits(s_event_group, TCP_CONNECTED_BIT);
        return -1;
    }

    ESP_LOGI(TAG, "Successfully connected to server");
    xEventGroupSetBits(s_event_group, TCP_CONNECTED_BIT);
    return 0;
}

static void send_data(float ax, float ay, float az, float gx, float gy, float gz, float temp)
{
    if (sock < 0) {
        ESP_LOGW(TAG, "Socket not connected, attempting to reconnect...");
        if (tcp_connect() != 0) {
            return;
        }
    }

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
             ax, ay, az, gx, gy, gz, temp);

    int err = send(sock, buffer, strlen(buffer), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        close(sock);
        sock = -1;
        xEventGroupClearBits(s_event_group, TCP_CONNECTED_BIT);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting MPU6050 WiFi transmission example...");

    s_event_group = xEventGroupCreate();

    wifi_init();

    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected!");

    ESP_LOGI(TAG, "Waiting for TCP connection...");
    while (tcp_connect() != 0) {
        ESP_LOGW(TAG, "Retrying TCP connection in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    ESP_LOGI(TAG, "TCP connected!");

    ESP_ERROR_CHECK(mpu6050_i2c_init());
    ESP_ERROR_CHECK(mpu6050_init());

    ESP_LOGI(TAG, "MPU6050 setup complete, starting data read loop...");

    float ax, ay, az;
    float gx, gy, gz;
    float temp;

    while (1) {
        EventBits_t bits = xEventGroupGetBits(s_event_group);
        if (!(bits & WIFI_CONNECTED_BIT) || !(bits & TCP_CONNECTED_BIT)) {
            ESP_LOGW(TAG, "Connection lost, waiting for reconnection...");
            xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT | TCP_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            ESP_LOGI(TAG, "Reconnected!");
        }

        if (mpu6050_read_accel(&ax, &ay, &az) == ESP_OK &&
            mpu6050_read_gyro(&gx, &gy, &gz) == ESP_OK &&
            mpu6050_read_temp(&temp) == ESP_OK) {
            
            ESP_LOGI(TAG, "Accel: X=%.3fg, Y=%.3fg, Z=%.3fg", ax, ay, az);
            ESP_LOGI(TAG, "Gyro:  X=%.3fdps, Y=%.3fdps, Z=%.3fdps", gx, gy, gz);
            ESP_LOGI(TAG, "Temp:  %.2f°C", temp);
            
            send_data(ax, ay, az, gx, gy, gz, temp);
        } else {
            ESP_LOGE(TAG, "Failed to read MPU6050 data");
        }

        ESP_LOGI(TAG, "----------------------------------------");

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
