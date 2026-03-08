#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_timer.h"  // 新增：引入esp_timer头文件（解决esp_timer_get_time报错）
#include "lwip/netdb.h"

// 引入拆分后的驱动头文件
#include "wifi_conn.h"
#include "dht11.h"

/* ==================== 配置区域 ==================== */
#define BUTTON_GPIO    GPIO_NUM_11
#define COZE_URL       "https://4npkk23hhg.coze.site/stream_run"
#define COZE_SESSION   "ESP32_S3_CUSTOM_SESSION_001"
#define COZE_PROJECT   "7610242979017719860"
static const char *COZE_TOKEN = "eyJhbGciOiJSUzI1NiIsImtpZCI6IjIzMjMwOGIyLWJkMTMtNGE4Mi1iZWJmLTcxMWZiMTQ2NThiOCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbIlZSa3NUUGoxMHpxa2lqUlVzY3M4YlRiRG1qblFrd2hhIl0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzcyOTM0MDI3LCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjEwMjQ4MDMyNzg0NzQ0NDgzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjE0NjkzNjY1NDAyNzgxNzM5In0.gbFGfHR09me1E2wTQ_yVaMrTXfNmGlWE6mBRzgEixlZj7M-HVhrVLoopZ9W-YWmxekwYj9YF4B-KOojD7hYjqAXeGJqIbN8A-tlsQt36xZA18ka2_Mjb0gVQfMEVuSpN1EyA4pjjxlPT1v64VbXKGjYEI2qlZKqe-YUXeoLttN7WWAWinQ29T1KffUmgT1Dv38Jf9Cg7X_KqgwLa4qDin8rCNhOgtdNvm9uJ484jB8FTVX9Z2TEiuwANWwbGISNMdw2-oCYiNHI0taTsUMfQufTqqwByBtuxTITCk9tyxqsGqlTvEdKwTvxyGIdRn6cgxwdvtRT5nTkJLqCohkKkwQ";

/* ==================== 全局变量 ==================== */
static const char *TAG = "ESP32_COZE";
static QueueHandle_t g_button_evt_queue = NULL;

/* ==================== HTTP事件回调 ==================== */
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR: ESP_LOGD(TAG, "HTTP_EVENT_ERROR"); break;
        case HTTP_EVENT_ON_CONNECTED: ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED"); break;
        case HTTP_EVENT_HEADER_SENT: ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT"); break;
        case HTTP_EVENT_ON_HEADER: ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value); break;
        case HTTP_EVENT_ON_DATA: ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len); break;
        case HTTP_EVENT_ON_FINISH: ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH"); break;
        case HTTP_EVENT_DISCONNECTED: ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED"); break;
        case HTTP_EVENT_REDIRECT: ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT"); break;
    }
    return ESP_OK;
}

/* ==================== 按键驱动 ==================== */
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
    ESP_LOGI(TAG, "Button initialized on GPIO%d", BUTTON_GPIO);
}

/* ==================== Coze API请求（最终修复版：防卡死 + 适配SSE） ==================== */
void test_tcp_connect(const char *host, uint16_t port) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        ESP_LOGE(TAG, "DNS失败: %s", host);
        return;
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket创建失败");
        return;
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };
    memcpy(&addr.sin_addr.s_addr, he->h_addr, he->h_length);
    struct timeval timeout = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) {
        ESP_LOGI(TAG, "连接 %s:%d 成功", host, port);
        close(sock);
    } else {
        ESP_LOGE(TAG, "连接 %s:%d 失败, errno=%d (%s)", host, port, errno, strerror(errno));
        close(sock);
    }
}
// 新增：检测DNS解析 + 端口连通性
static void check_network_connectivity(const char* host, uint16_t port) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        ESP_LOGE(TAG, "DNS解析失败！host=%s, errno=%d", host, errno);
        return;
    }
    ESP_LOGI(TAG, "DNS解析成功: %s -> " IPSTR, host, IP2STR((ip4_addr_t*)he->h_addr));

    // 尝试建立TCP连接（测试端口是否通）
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "创建socket失败: %d", errno);
        return;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, he->h_addr, he->h_length);

    // 设置socket超时（5秒）
    struct timeval timeout = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    ESP_LOGI(TAG, "尝试连接 %s:%d...", host, port);
    int ret = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == 0) {
        ESP_LOGI(TAG, "TCP连接成功！");
        close(sock);
    } else {
        ESP_LOGE(TAG, "TCP连接失败！errno=%d, 原因：%s", errno, strerror(errno));
        close(sock);
    }
}
static void coze_send_request(float temp, float humid) {
    // 1. 前置检查
    if (COZE_TOKEN == NULL || strlen(COZE_TOKEN) == 0) {
        ESP_LOGE(TAG, "Coze Error: Token为空");
        return;
    }
    if (!wifi_is_connected()) {
        ESP_LOGE(TAG, "Coze Error: WiFi未连接");
        return;
    }

    test_tcp_connect("www.baidu.com", 80);
    check_network_connectivity("4npkk23hhg.coze.site", 443); // Coze域名+HTTPS端口

    // 2. 生成提示词
    char prompt_text[128] = {0};
    snprintf(prompt_text, sizeof(prompt_text), "温度:%.1f℃,湿度:%.1f%%。请给出生活建议。", temp, humid);
    ESP_LOGI(TAG, "Coze prompt: %s", prompt_text);

    // 3. 构建JSON请求体（和Python完全一致）
    cJSON *root = cJSON_CreateObject();          
    cJSON *content = cJSON_CreateObject();       
    cJSON *query = cJSON_CreateObject();         
    cJSON *prompt_array = cJSON_CreateArray();   
    cJSON *prompt_item = cJSON_CreateObject();   
    cJSON *prompt_content = cJSON_CreateObject();

    cJSON_AddStringToObject(prompt_content, "text", prompt_text);
    cJSON_AddStringToObject(prompt_item, "type", "text");
    cJSON_AddItemToObject(prompt_item, "content", prompt_content);
    cJSON_AddItemToArray(prompt_array, prompt_item);
    cJSON_AddItemToObject(query, "prompt", prompt_array);
    cJSON_AddItemToObject(content, "query", query);

    cJSON_AddStringToObject(root, "type", "query");
    cJSON_AddStringToObject(root, "session_id", COZE_SESSION);
    cJSON_AddStringToObject(root, "project_id", COZE_PROJECT);
    cJSON_AddItemToObject(root, "content", content);

    char *post_data = cJSON_PrintUnformatted(root);
    if (!post_data) {
        ESP_LOGE(TAG, "JSON生成失败");
        cJSON_Delete(root);
        return;
    }

    // 4. HTTP客户端配置（核心：防阻塞 + 短超时）
    esp_http_client_config_t config = {
    .url = COZE_URL,
    .method = HTTP_METHOD_POST,
    .timeout_ms = 15000,                // 连接超时从8s增至15s
    .buffer_size = 4096,                // 增大缓冲区
    .buffer_size_tx = 4096,
    .user_agent = "ESP32S3/1.0",
    .keep_alive_enable = true,         //长连接
    .skip_cert_common_name_check = true,
    .disable_auto_redirect = true,
    .event_handler = http_event_handler,
    .max_redirection_count = 0,
    .use_global_ca_store = false,
    .cert_pem = NULL, // 不加载自定义证书
    .port = 443,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP客户端初始化失败");
        free(post_data);
        cJSON_Delete(root);
        return;
    }

    // 5. 设置请求头（和Python脚本100%对齐）
    char auth_header[1024] = {0};
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", COZE_TOKEN);
    esp_http_client_set_header(client, "Host", "4npkk23hhg.coze.site");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");    // 关键：指定SSE格式
    esp_http_client_set_header(client, "Cache-Control", "no-cache");
    esp_http_client_set_header(client, "Connection", "keep-alive");             // 关键：用完关闭连接

    // 6. 设置POST数据并发送请求
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "请求发送失败: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // 7. 检查响应状态码
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Coze响应状态码: %d", status_code);
    if (status_code != 200) {
        ESP_LOGE(TAG, "服务端返回非200状态码: %d", status_code);
        goto cleanup;
    }

    // 8. 核心：流式读取SSE数据（防卡死 + 20秒超时）
printf("\n========== Coze智能体回复 ==========\n");
char read_buf[256] = {0};
int total_read = 0;
int64_t start_time = esp_timer_get_time() / 1000;  // 记录开始时间（毫秒）

while (1) {
    // 超时保护：从10秒增至20秒，适配服务端响应速度
    int64_t current_time = esp_timer_get_time() / 1000;
    if (current_time - start_time > 20000) {
        ESP_LOGW(TAG, "SSE读取超时（20秒）");
        break;
    }

    // 单次读取128字节，预留1字节给字符串结束符
    int read_len = esp_http_client_read(client, read_buf, sizeof(read_buf) - 1);
    ESP_LOGD(TAG, "SSE单次读取长度: %d", read_len); // 新增：打印读取长度，排查问题

    if (read_len > 0) {
        read_buf[read_len] = '\0';  // 确保字符串结束
        total_read += read_len;
        
        // 直接打印原始数据（不做复杂解析，避免卡死）
        printf("%s", read_buf);
        fflush(stdout);  // 强制刷新输出，立刻显示
        
        // 检测结束标记 [DONE]，收到后立即退出
        if (strstr(read_buf, "[DONE]")) {
            ESP_LOGI(TAG, "收到结束标记，停止读取");
            break;
        }

        memset(read_buf, 0, sizeof(read_buf));  // 清空缓冲区
    } else if (read_len == 0) {
        // 无数据时等待50ms，减少空轮询（原20ms太频繁）
        vTaskDelay(pdMS_TO_TICKS(50));
    } else {
        // 读取错误，打印具体错误码
        ESP_LOGE(TAG, "SSE读取错误，错误码: %d", read_len);
        break;
    }
}

    printf("\n========== 回复结束 ==========\n");
    ESP_LOGI(TAG, "SSE读取完成，累计读取: %d字节", total_read);

cleanup:
    // 资源清理
    esp_http_client_cleanup(client);
    free(post_data);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Coze请求流程结束");
}

/* ==================== 主任务 ==================== */
static void main_task(void *pvParameters) {
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(g_button_evt_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(50)); // 消抖
            if (gpio_get_level(io_num) == 0) { 
                if (!wifi_is_connected()) {
                    ESP_LOGW(TAG, "WiFi未连接，跳过请求");
                    continue;
                }

                // 二次确认WiFi关联状态
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
                    ESP_LOGW(TAG, "WiFi未关联到热点");
                    continue;
                }

                // 读取DHT11数据（重试3次）
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

/* ==================== 入口函数 ==================== */
void app_main(void) {
    // 设置日志级别
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_DEBUG);
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_DEBUG);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE); // 打印SSL详细日志
    esp_log_level_set("tls_transport", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "系统启动中...");
    
    // 初始化各模块
    wifi_conn_init();
    button_init();
    dht11_init();

    // 创建主任务
    xTaskCreate(main_task, "main_task", 32768, NULL, 5, NULL);
}