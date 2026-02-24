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

/* ==================== 配置区域 (请根据实际情况修改) ==================== */
#define WIFI_SSID      "zhangfanding"
#define WIFI_PASS      "12345678"
#define BUTTON_GPIO    GPIO_NUM_0    // 按键引脚 (默认BOOT按键)
#define DHT11_GPIO     GPIO_NUM_4    // DHT11数据引脚
#define COZE_URL       "https://4npkk23hhg.coze.site/stream_run"
#define COZE_TOKEN     "eyJhbGciOiJSUzI1NiIsImtpZCI6IjIxYTNiY2FlLWRiZGEtNGI4NS04YjI3LWNmNTg3YzFjNGFjMCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbIlZSa3NUUGoxMHpxa2lqUlVzY3M4YlRiRG1qblFrd2hhIl0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzcxOTAwODgzLCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjEwMjQ4MDMyNzg0NzQ0NDgzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjEwMjU2MzQ1MTQ0NDkyMDg0In0.eLkKnGtcmDEX-78mnbD99L7jPBM_y5uU43lGrWaYSXFpiVM6r2zDnTcmRrpwUw47HyPeRNv8HR7lPkRxJJMmKOKSwKP3LkP3dCvkDWYc-jDTwmBwENtryHPE8YVkUMChiUp2eTfDFKf-LWH5L8NxM5yPWakh_kuVzvNmON05bovMWu1_JA7l43TufEn4RqorvCufisKN5KB6A3Xy42uqLOZ3Q3CbK1pUOi4BAJYx2VeOGi72OLIRwvs31QbT0RjbnSjljqnNH_-znqW3EBLYJzYcavPxduanBzWW41TnWqYJLwvvRGg66JLKZjvVOQlPiAafQxitZWbozGCH9oZqqA" // 注意：不要加"Bearer "前缀
#define COZE_SESSION   "7Yd_AbKVJ3vkudX0OlF6h"
#define COZE_PROJECT   "7610242979017719860"

/* ==================== 全局变量与日志标签 ==================== */
static const char *TAG = "ESP32_COZE";
static QueueHandle_t g_button_evt_queue = NULL;
static bool g_wifi_connected = false;

/* ==================== DHT11驱动代码 ==================== */
static void dht11_gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DHT11_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

static int dht11_read(float *temp, float *humid) {
    uint8_t data[5] = {0};
    
    // 1. 主机发送起始信号
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    sys_delay_ms(20); // 拉低20ms
    gpio_set_level(DHT11_GPIO, 1);
    sys_delay_ms(1);    // 拉高40us
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);

    // 2. 等待DHT11响应 (80us低 + 80us高)
    uint32_t timeout = 10000;
    while(gpio_get_level(DHT11_GPIO) == 1) if(--timeout == 0) return -1;
    timeout = 10000;
    while(gpio_get_level(DHT11_GPIO) == 0) if(--timeout == 0) return -1;
    timeout = 10000;
    while(gpio_get_level(DHT11_GPIO) == 1) if(--timeout == 0) return -1;

    // 3. 读取40bit数据
    for(int i = 0; i < 40; i++) {
        // 等待50us低电平开始
        timeout = 10000;
        while(gpio_get_level(DHT11_GPIO) == 0) if(--timeout == 0) return -1;
        // 检测高电平长度 (26-28us=0, 70us=1)
        sys_delay_ms(1);
        data[i/8] <<= 1;
        if(gpio_get_level(DHT11_GPIO) == 1) data[i/8] |= 1;
        // 等待剩余高电平结束
        timeout = 10000;
        while(gpio_get_level(DHT11_GPIO) == 1) if(--timeout == 0) return -1;
    }

    // 4. 校验和验证
    if(data[4] != (data[0] + data[1] + data[2] + data[3])) return -1;
    
    *humid = data[0] + (float)data[1]/10.0f;
    *temp  = data[2] + (float)data[3]/10.0f;
    return 0;
}

/* ==================== WiFi连接代码 ==================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_connected = false;
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
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

/* ==================== 按键驱动代码 (带消抖) ==================== */
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
        .intr_type = GPIO_INTR_NEGEDGE // 下降沿触发 (按下)
    };
    gpio_config(&io_conf);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void*) BUTTON_GPIO);
}

/* ==================== 扣子API请求与SSE解析 (更新版) ==================== */

// 用于SSE解析的状态机
typedef enum {
    SSE_STATE_IDLE,        // 等待新行
    SSE_STATE_READ_EVENT,  // 刚读到 "event:"
    SSE_STATE_READ_DATA,   // 刚读到 "data:"
} sse_state_t;

static void coze_send_request(float temp, float humid) {
    // 1. 构建提示词
    char prompt[128];
    snprintf(prompt, sizeof(prompt), "温度:%.1f℃,湿度:%.1f%%。请给出生活建议。", temp, humid);

    // 2. 构建JSON请求体
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

    // 3. 配置HTTP客户端
    esp_http_client_config_t config = {
        .url = COZE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000, // 超时时间设长一点
        .buffer_size = 4096,
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 4. 设置Header
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", COZE_TOKEN);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");

    // 5. 发送请求头
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Connection failed: %s", esp_err_to_name(err));
        goto cleanup;
    }
    
    // 写入Body
    int wlen = esp_http_client_write(client, post_data, strlen(post_data));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Write failed");
        goto cleanup;
    }

    // 6. 读取响应头
    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP Status != 200: %d", status_code);
        // 尝试读取错误信息
        char err_buf[512];
        esp_http_client_read(client, err_buf, sizeof(err_buf)-1);
        ESP_LOGE(TAG, "Error content: %s", err_buf);
        goto cleanup;
    }

    // 7. 流式解析SSE数据
    printf("\n========== 智能体回复 ==========\n");

    // 解析缓冲区
    #define SSE_BUF_SIZE (1024 * 2)
    char *parse_buf = malloc(SSE_BUF_SIZE);
    int parse_idx = 0;
    
    char current_event_type[64] = {0}; // 存储当前的 event 类型
    bool is_running = true;

    while (is_running) {
        // 读取一段数据到临时缓冲区
        char read_buf[512];
        int len = esp_http_client_read(client, read_buf, sizeof(read_buf) - 1);
        
        if (len < 0) {
            ESP_LOGE(TAG, "Read error");
            break;
        }
        if (len == 0) {
            // 读取完毕或连接关闭
            break;
        }

        // 将新数据追加到解析缓冲区 (简单的字节流处理)
        for (int i = 0; i < len; i++) {
            char c = read_buf[i];
            
            // 检查行结束符
            if (c == '\n') {
                parse_buf[parse_idx] = '\0'; // 字符串结束
                
                // ---- 解析这一行 ----
                char *line = parse_buf;
                
                // 去除开头的 \r (如果是 \r\n 结尾)
                if (parse_idx > 0 && parse_buf[parse_idx-1] == '\r') {
                    parse_buf[parse_idx-1] = '\0';
                }

                if (strncmp(line, "event:", 6) == 0) {
                    // 提取事件类型: "event: answer" -> "answer"
                    strncpy(current_event_type, line + 6, sizeof(current_event_type) - 1);
                    // 去除前面可能的空格
                    char *trimmed = current_event_type;
                    while(*trimmed == ' ') trimmed++;
                    memmove(current_event_type, trimmed, strlen(trimmed)+1);
                } 
                else if (strncmp(line, "data:", 5) == 0) {
                    // 提取数据内容
                    char *json_str = line + 5;
                    while(*json_str == ' ') json_str++; // 去空格
                    
                    // 只有当 event 是我们关心的类型时才解析JSON
                    if (strlen(current_event_type) > 0) {
                        cJSON *json = cJSON_Parse(json_str);
                        if (json) {
                            // 3.1 处理 answer 事件 (增量内容)
                            if (strcmp(current_event_type, "answer") == 0) {
                                cJSON *answer = cJSON_GetObjectItem(json, "answer");
                                if (cJSON_IsString(answer)) {
                                    // 直接打印增量文本，不换行
                                    printf("%s", answer->valuestring);
                                    fflush(stdout); // 强制刷新缓冲区
                                }
                            }
                            // 3.2 处理 message_end 事件 (结束标志)
                            else if (strcmp(current_event_type, "message_end") == 0) {
                                cJSON *code = cJSON_GetObjectItem(json, "code");
                                if (cJSON_IsString(code) && strcmp(code->valuestring, "0") == 0) {
                                    // 成功结束
                                    is_running = false;
                                } else {
                                    ESP_LOGW(TAG, "Message ended with non-zero code.");
                                    is_running = false;
                                }
                            }
                            // 3.3 处理 error 事件
                            else if (strcmp(current_event_type, "error") == 0) {
                                cJSON *err_msg = cJSON_GetObjectItem(json, "error_msg");
                                if (cJSON_IsString(err_msg)) {
                                    ESP_LOGE(TAG, "API Error: %s", err_msg->valuestring);
                                }
                                is_running = false;
                            }
                            // 其他事件 (message_start, tool_* 等) 可以在这里添加逻辑，或者忽略
                            
                            cJSON_Delete(json);
                        }
                    }
                }
                else if (strlen(line) == 0) {
                    // 空行，SSE标准中表示一个事件块结束，重置状态
                    current_event_type[0] = '\0'; 
                }

                // 重置索引，处理下一行
                parse_idx = 0;
            } else {
                // 普通字符，存入缓冲区
                if (parse_idx < SSE_BUF_SIZE - 1) {
                    parse_buf[parse_idx++] = c;
                }
            }
        }
    }

    free(parse_buf);
    printf("\n========== 回复结束 ==========\n\n");

cleanup:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(post_data);
}

/* ==================== 主任务 ==================== */
static void main_task(void *pvParameters) {
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(g_button_evt_queue, &io_num, portMAX_DELAY)) {
            // 软件消抖：延时20ms后再次检测电平
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(io_num) == 0) { 
                ESP_LOGI(TAG, "Button pressed!");

                if (!g_wifi_connected) {
                    ESP_LOGW(TAG, "WiFi not connected yet.");
                    continue;
                }

                // 读取温湿度
                float temp, humid;
                if (dht11_read(&temp, &humid) == 0) {
                    ESP_LOGI(TAG, "Read: T=%.1f H=%.1f", temp, humid);
                    // 发送请求
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
    ESP_LOGI(TAG, "System starting...");
    
    wifi_init();
    button_init();
    dht11_gpio_init();
    
    xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
}