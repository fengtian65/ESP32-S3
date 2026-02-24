#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_rom_sys.h"
#include "esp_intr_alloc.h"

/* ==================== 配置区域 ==================== */
#define WIFI_SSID      "你的WiFi名称"
#define WIFI_PASS      "你的WiFi密码"
#define BUTTON_GPIO    GPIO_NUM_0
#define DHT11_GPIO     GPIO_NUM_4
#define COZE_URL       "https://4npkk23hhg.coze.site/stream_run"
#define COZE_SESSION   "7Yd_AbKVJ3vkudX0OlF6h"
#define COZE_PROJECT   "7610242979017719860"

// 注意：Token太长，建议在下方代码中直接赋值，而不是在宏定义里写超长字符串
// 这里声明为外部变量，在app_main中赋值
static const char *COZE_TOKEN = NULL;

/* ==================== 全局变量 ==================== */
static const char *TAG = "ESP32_COZE";
static QueueHandle_t g_button_evt_queue = NULL;
static bool g_wifi_connected = false;

/* ==================== DHT11驱动 ==================== */
static void dht11_gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DHT11_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// 替换为：
static inline void dht11_delay_us(uint32_t us) {
    // 为了简化兼容性，这里直接使用 esp_rom_delay_us
    // 注：DHT11对时序敏感，建议不要在读取时打开WiFi日志或进行其他耗时操作
    esp_rom_delay_us(us);
}
static int dht11_read(float *temp, float *humid) {
    uint8_t data[5] = {0};
    uint32_t timeout = 0;
    
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    dht11_delay_us(20000);
    gpio_set_level(DHT11_GPIO, 1);
    dht11_delay_us(40);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);

    timeout = 100000;
    while(gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
    if(timeout == 0) return -1;

    timeout = 100000;
    while(gpio_get_level(DHT11_GPIO) == 0 && timeout--) dht11_delay_us(1);
    if(timeout == 0) return -1;

    timeout = 100000;
    while(gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
    if(timeout == 0) return -1;

    for(int i = 0; i < 40; i++) {
        timeout = 100000;
        while(gpio_get_level(DHT11_GPIO) == 0 && timeout--) dht11_delay_us(1);
        if(timeout == 0) return -1;
        
        dht11_delay_us(40);
        data[i/8] <<= 1;
        if(gpio_get_level(DHT11_GPIO) == 1) data[i/8] |= 1;
        
        timeout = 100000;
        while(gpio_get_level(DHT11_GPIO) == 1 && timeout--) dht11_delay_us(1);
        if(timeout == 0) return -1;
    }

    if(data[4] != (data[0] + data[1] + data[2] + data[3])) return -1;
    *humid = data[0] + (float)data[1]/10.0f;
    *temp  = data[2] + (float)data[3]/10.0f;
    return 0;
}

/* ==================== WiFi ==================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_wifi_connected = true;
        ESP_LOGI(TAG, "WiFi connected!");
    }
}

static void wifi_init(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

/* ==================== 按键 ==================== */
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(g_button_evt_queue, &gpio_num, NULL);
}

static void button_init(void) {
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
}

/* ==================== 扣子API请求 ==================== */
static void coze_send_request(float temp, float humid) {
    if (COZE_TOKEN == NULL) {
        ESP_LOGE(TAG, "Token not set!");
        return;
    }

    char prompt[128];
    snprintf(prompt, sizeof(prompt), "温度:%.1f℃,湿度:%.1f%%。请给出生活建议。", temp, humid);

    cJSON *root = cJSON_CreateObject();
    cJSON *content = cJSON_CreateObject();
    cJSON *query = cJSON_CreateObject();
    cJSON *prompt_arr = cJSON_CreateArray();
    cJSON *prompt_obj = cJSON_CreateObject();
    cJSON *text_content = cJSON_CreateObject();

    cJSON_AddStringToObject(prompt_obj, "type", "text");
    cJSON_AddStringToObject(text_content, "text", prompt);
    cJSON_AddItemToObject(prompt_obj, "content", text_content);
    cJSON_AddItemToArray(prompt_arr, prompt_obj);
    cJSON_AddItemToObject(query, "prompt", prompt_arr);
    cJSON_AddItemToObject(content, "query", query);
    cJSON_AddItemToObject(root, "content", content);
    cJSON_AddStringToObject(root, "type", "query");
    cJSON_AddStringToObject(root, "session_id", COZE_SESSION);
    cJSON_AddStringToObject(root, "project_id", COZE_PROJECT);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    esp_http_client_config_t config = {
        .url = COZE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .buffer_size = 4096,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 关键修复：这里的缓冲区要足够大 (1024字节)
    char auth_header[1024]; 
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", COZE_TOKEN);
    
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    
    if (err != ESP_OK || esp_http_client_get_status_code(client) != 200) {
        ESP_LOGE(TAG, "Request failed");
        goto cleanup;
    }

    printf("\n========== 智能体回复 ==========\n");
    #define SSE_BUF_SIZE 2048
    char *parse_buf = malloc(SSE_BUF_SIZE);
    int parse_idx = 0;
    char current_event_type[64] = {0};
    
    char read_buf[512];
    int len = 0;
    while ((len = esp_http_client_read(client, read_buf, sizeof(read_buf)-1)) > 0) {
        for (int i = 0; i < len; i++) {
            char c = read_buf[i];
            if (c == '\n') {
                parse_buf[parse_idx] = '\0';
                if (parse_idx > 0 && parse_buf[parse_idx-1] == '\r') parse_buf[parse_idx-1] = '\0';
                
                if (strncmp(parse_buf, "event:", 6) == 0) {
                    strncpy(current_event_type, parse_buf + 6, sizeof(current_event_type)-1);
                    char *trimmed = current_event_type; while(*trimmed == ' ') trimmed++;
                    memmove(current_event_type, trimmed, strlen(trimmed)+1);
                } 
                else if (strncmp(parse_buf, "data:", 5) == 0) {
                    char *json_str = parse_buf + 5;
                    while(*json_str == ' ') json_str++;
                    cJSON *json = cJSON_Parse(json_str);
                    if (json) {
                        if (strcmp(current_event_type, "answer") == 0) {
                            cJSON *answer = cJSON_GetObjectItem(json, "answer");
                            if (cJSON_IsString(answer)) {
                                printf("%s", answer->valuestring);
                                fflush(stdout);
                            }
                        }
                        cJSON_Delete(json);
                    }
                }
                parse_idx = 0;
            } else {
                if (parse_idx < SSE_BUF_SIZE - 1) parse_buf[parse_idx++] = c;
            }
        }
    }
    free(parse_buf);
    printf("\n========== 回复结束 ==========\n\n");

cleanup:
    esp_http_client_cleanup(client);
    free(post_data);
}

/* ==================== 主任务 ==================== */
static void main_task(void *pvParameters) {
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(g_button_evt_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(io_num) == 0) { 
                if (!g_wifi_connected) {
                    ESP_LOGW(TAG, "WiFi not connected.");
                    continue;
                }
                float temp, humid;
                if (dht11_read(&temp, &humid) == 0) {
                    ESP_LOGI(TAG, "Read: T=%.1f H=%.1f", temp, humid);
                    coze_send_request(temp, humid);
                } else {
                    ESP_LOGE(TAG, "DHT11 read failed!");
                }
            }
        }
    }
}

/* ==================== 入口函数 ==================== */
void app_main(void) {
    // 在这里粘贴你的超长Token，避免宏定义报错
    COZE_TOKEN = "eyJhbGciOiJSUzI1NiIsImtpZCI6IjIxYTNiY2FlLWRiZGEtNGI4NS04YjI3LWNmNTg3YzFjNGFjMCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbIlZSa3NUUGoxMHpxa2lqUlVzY3M4YlRiRG1qblFrd2hhIl0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzcxOTAwODgzLCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjEwMjQ4MDMyNzg0NzQ0NDgzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjEwMjU2MzQ1MTQ0NDkyMDg0In0.eLkKnGtcmDEX-78mnbD99L7jPBM_y5uU43lGrWaYSXFpiVM6r2zDnTcmRrpwUw47HyPeRNv8HR7lPkRxJJMmKOKSwKP3LkP3dCvkDWYc-jDTwmBwENtryHPE8YVkUMChiUp2eTfDFKf-LWH5L8NxM5yPWakh_kuVzvNmON05bovMWu1_JA7l43TufEn4RqorvCufisKN5KB6A3Xy42uqLOZ3Q3CbK1pUOi4BAJYx2VeOGi72OLIRwvs31QbT0RjbnSjljqnNH_-znqW3EBLYJzYcavPxduanBzWW41TnWqYJLwvvRGg66JLKZjvVOQlPiAafQxitZWbozGCH9oZqqA";

    ESP_LOGI(TAG, "System starting...");
    wifi_init();
    button_init();
    dht11_gpio_init();
    xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
}