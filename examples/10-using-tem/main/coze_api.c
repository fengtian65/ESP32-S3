#include "coze_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_timer.h"
#include "lwip/netdb.h"
#include "wifi_conn.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "COZE_API";

// ========== 配置常量（可在此修改）==========
#define COZE_URL       "https://pcgrcsptjq.coze.site/stream_run"
#define COZE_SESSION   "ESP32_S3_CUSTOM_SESSION_001"
#define COZE_PROJECT   "7616311002971521087"
static const char *COZE_TOKEN = "eyJhbGciOiJSUzI1NiIsImtpZCI6Ijg4OTdjOGE0LTc5ZDEtNDI0My04ZmZmLTRhNjQzNDgwYjRhNCJ9.eyJpc3MiOiJodHRwczovL2FwaS5jb3plLmNuIiwiYXVkIjpbInJLSFJuNmo3Q3FQZmtKVkJGOWg5SWFCVzNNWkluNkY1Il0sImV4cCI6ODIxMDI2Njg3Njc5OSwiaWF0IjoxNzczMzEyNjQ4LCJzdWIiOiJzcGlmZmU6Ly9hcGkuY296ZS5jbi93b3JrbG9hZF9pZGVudGl0eS9pZDo3NjE2MzE3MDYxNDIxMDcyNDAzIiwic3JjIjoiaW5ib3VuZF9hdXRoX2FjY2Vzc190b2tlbl9pZDo3NjE2MzE5ODI5NDQzNjA4NjI2In0.AuVW5O8hwAHRK8pK438a_JHSMZTbfmemntjpoKopKVn6LZZeoxaWRe7helMaHkFnpXd6Woiw4e9YUobaS0V94kPp9TbBhYxns1a24QeKUfg3pdRLrHF5vojAdonVGZ_PuuzZPyydpeRWKcTG53y6PQVmOCufyejNuZjVHR_4xPU1hUldmssc08XYfg7ZuRDgm_l1scSESqcLEUFVzT2Nds9Nqgw5n_3ZqVVo7PorNQCzQU61s61s8gxYy2dRMzW1ApfRent2J-ktze_VogYwum4o1RRJtDGDWctDWgh5pmX0_dhZnknxlifhGdDlFIpUzXhLJYmVBqdjg23ffWAWHA";
// ========== 内部函数声明 ==========
static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static void test_tcp_connect(const char *host, uint16_t port);
static void check_network_connectivity(const char* host, uint16_t port);

// ========== HTTP 事件回调 ==========
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

// ========== 网络连通性测试 ==========
static void test_tcp_connect(const char *host, uint16_t port) {
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

static void check_network_connectivity(const char* host, uint16_t port) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        ESP_LOGE(TAG, "DNS解析失败！host=%s, errno=%d", host, errno);
        return;
    }
    ESP_LOGI(TAG, "DNS解析成功: %s -> %s", host, ip4addr_ntoa((const ip4_addr_t*)he->h_addr));

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

// ========== 主请求函数 ==========
void coze_send_request(float temp, float humid) {
    // 前置检查
    if (COZE_TOKEN == NULL || strlen(COZE_TOKEN) == 0) {
        ESP_LOGE(TAG, "Coze Error: Token为空");
        return;
    }
    if (!wifi_is_connected()) {
        ESP_LOGE(TAG, "Coze Error: WiFi未连接");
        return;
    }

    // 可选：测试连通性
    //test_tcp_connect("www.baidu.com", 80);
    //check_network_connectivity("cfq6w5hsx2.coze.site", 443);

    // 构建提示词
    char prompt_text[128] = {0};
    snprintf(prompt_text, sizeof(prompt_text), "温度:%.1f℃,湿度:%.1f%%。请给出生活建议。", temp, humid);
    ESP_LOGI(TAG, "Coze prompt: %s", prompt_text);

    // 构建 JSON 请求体
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

    // HTTP 客户端配置
    esp_http_client_config_t config = {
        .url = COZE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .buffer_size = 4096,
        .buffer_size_tx = 4096,
        .user_agent = "ESP32S3/1.0",
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,
        .disable_auto_redirect = true,
        .event_handler = http_event_handler,
        .max_redirection_count = 0,
        .use_global_ca_store = false,
        .cert_pem = NULL,
        .port = 443,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "HTTP客户端初始化失败");
        free(post_data);
        cJSON_Delete(root);
        return;
    }

    // 设置请求头
    char auth_header[1024] = {0};
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", COZE_TOKEN);
    esp_http_client_set_header(client, "Host", "pcgrcsptjq.coze.site");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "text/event-stream");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");
    esp_http_client_set_header(client, "Connection", "keep-alive");

    // 分步请求
    esp_err_t err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_http_client_open 失败: %s", esp_err_to_name(err));
        goto cleanup;
    }

    int written = esp_http_client_write(client, post_data, strlen(post_data));
    if (written != strlen(post_data)) {
        ESP_LOGE(TAG, "请求体发送不完整, 写入 %d 字节", written);
        goto cleanup;
    }

    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "Content-Length: %d", content_length);

    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Coze响应状态码: %d", status_code);
    if (status_code != 200) {
        ESP_LOGE(TAG, "服务端返回非200状态码: %d", status_code);
        goto cleanup;
    }

    // 读取 SSE 流式数据并拼接 answer
    printf("\n========== Coze智能体回复 ==========\n");

    char read_buf[256];
    int total_read = 0;
    int64_t start_time = esp_timer_get_time() / 1000;

    char *full_answer = malloc(1);
    if (!full_answer) {
        ESP_LOGE(TAG, "内存分配失败");
        goto cleanup;
    }
    full_answer[0] = '\0';
    size_t answer_len = 0;
    size_t answer_cap = 1;

    char line_buf[512];
    int line_len = 0;
    bool done_received = false;

    while (!done_received) {
        int64_t current_time = esp_timer_get_time() / 1000;
        if (current_time - start_time > 20000) {
            ESP_LOGW(TAG, "SSE读取超时（20秒）");
            break;
        }

        int read_len = esp_http_client_read(client, read_buf, sizeof(read_buf) - 1);
        if (read_len > 0) {
            total_read += read_len;
            for (int i = 0; i < read_len; i++) {
                char c = read_buf[i];
                if (c == '\n') {
                    if (line_len > 0) {
                        line_buf[line_len] = '\0';
                        if (strncmp(line_buf, "data: ", 6) == 0) {
                            char *data = line_buf + 6;
                            if (strcmp(data, "[DONE]") == 0) {
                                ESP_LOGI(TAG, "收到结束标记 [DONE]");
                                done_received = true;
                                break;
                            } else {
                                cJSON *root = cJSON_Parse(data);
                                if (root) {
                                    cJSON *content = cJSON_GetObjectItem(root, "content");
                                    if (content) {
                                        cJSON *answer = cJSON_GetObjectItem(content, "answer");
                                        if (answer && cJSON_IsString(answer)) {
                                            const char *ans_str = answer->valuestring;
                                            size_t ans_len = strlen(ans_str);
                                            if (answer_cap <= answer_len + ans_len + 1) {
                                                answer_cap = answer_len + ans_len + 1024;
                                                char *new_ptr = realloc(full_answer, answer_cap);
                                                if (!new_ptr) {
                                                    ESP_LOGE(TAG, "realloc 失败");
                                                    cJSON_Delete(root);
                                                    goto cleanup;
                                                }
                                                full_answer = new_ptr;
                                            }
                                            memcpy(full_answer + answer_len, ans_str, ans_len);
                                            answer_len += ans_len;
                                            full_answer[answer_len] = '\0';
                                        }
                                    }
                                    cJSON_Delete(root);
                                }
                            }
                        }
                        line_len = 0;
                    }
                    if (done_received) break;
                } else {
                    if (line_len < (int)sizeof(line_buf) - 1) {
                        line_buf[line_len++] = c;
                    } else {
                        ESP_LOGW(TAG, "行过长，截断");
                        line_len = 0;
                    }
                }
            }
        } else if (read_len == 0) {
            if (esp_http_client_is_complete_data_received(client)) {
                ESP_LOGI(TAG, "所有数据接收完毕");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            ESP_LOGE(TAG, "SSE读取错误，错误码: %d", read_len);
            break;
        }
    }

    // 输出最终拼接的完整回答
    printf("\n========== Coze智能体完整回复 ==========\n");
    printf("%s\n", full_answer);
    printf("========== 回复结束 ==========\n");
    ESP_LOGI(TAG, "SSE读取完成，累计读取: %d字节", total_read);

    free(full_answer);

cleanup:
    esp_http_client_cleanup(client);
    free(post_data);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Coze请求流程结束");
}