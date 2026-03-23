#include <stdio.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "MAIN_APP";
static const char *TAG_SENSOR = "SENSOR";
static const char *TAG_NETWORK = "NETWORK";

void log_level_demo(void)
{
    ESP_LOGE(TAG, "这是一个ERROR级别的日志 - 用于错误信息");
    ESP_LOGW(TAG, "这是一个WARN级别的日志 - 用于警告信息");
    ESP_LOGI(TAG, "这是一个INFO级别的日志 - 用于一般信息");
    ESP_LOGD(TAG, "这是一个DEBUG级别的日志 - 用于调试信息");
    ESP_LOGV(TAG, "这是一个VERBOSE级别的日志 - 用于详细调试信息");
}

void log_format_demo(void)
{
    int value = 42;
    float temperature = 25.6;
    char message[] = "Hello ESP32";

    ESP_LOGI(TAG, "整数输出: %d", value);
    ESP_LOGI(TAG, "浮点数输出: %.2f", temperature);
    ESP_LOGI(TAG, "字符串输出: %s", message);
    ESP_LOGI(TAG, "十六进制输出: 0x%x", value);
    ESP_LOGI(TAG, "指针地址: %p", &value);
    ESP_LOGI(TAG, "多个参数: value=%d, temp=%.1f, msg=%s", value, temperature, message);
}

void log_hex_dump_demo(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    size_t len = sizeof(data);

    ESP_LOGI(TAG, "十六进制数据转储演示:");
    ESP_LOG_BUFFER_HEX(TAG, data, len);
    
    ESP_LOGI(TAG, "十六进制+ASCII数据转储:");
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
}

void log_timestamp_demo(void)
{
    ESP_LOGI(TAG, "带时间戳的日志演示");
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "循环计数: %d", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void log_color_demo(void)
{
    ESP_LOGI(TAG, "彩色日志演示");
    ESP_LOGE(TAG, "红色 - 错误信息");
    ESP_LOGW(TAG, "黄色 - 警告信息");
    ESP_LOGI(TAG, "绿色 - 一般信息");
    ESP_LOGD(TAG, "默认颜色 - 调试信息");
    ESP_LOGV(TAG, "默认颜色 - 详细信息");
}

void log_tag_demo(void)
{
    ESP_LOGI(TAG, "使用不同的日志标签");
    ESP_LOGI(TAG_SENSOR, "传感器数据读取中...");
    ESP_LOGI(TAG_NETWORK, "网络连接状态检查中...");
    ESP_LOGI(TAG, "主程序逻辑执行中...");
}

void log_level_control_demo(void)
{
    ESP_LOGI(TAG, "日志级别控制演示");
    
    esp_log_level_t current_level = esp_log_level_get(TAG);
    ESP_LOGI(TAG, "当前MAIN标签的日志级别: %d", current_level);
    
    ESP_LOGI(TAG, "设置MAIN标签为ERROR级别");
    esp_log_level_set(TAG, ESP_LOG_ERROR);
    
    ESP_LOGE(TAG, "这个ERROR日志会显示");
    ESP_LOGW(TAG, "这个WARN日志不会显示");
    ESP_LOGI(TAG, "这个INFO日志不会显示");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "恢复MAIN标签为INFO级别");
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    ESP_LOGI(TAG, "现在INFO级别的日志又可以显示了");
}

void log_global_level_demo(void)
{
    ESP_LOGI(TAG, "全局日志级别控制演示");
    
    ESP_LOGI(TAG, "设置全局日志级别为WARNING");
    esp_log_level_set("*", ESP_LOG_WARN);
    
    ESP_LOGE(TAG, "ERROR日志显示");
    ESP_LOGW(TAG, "WARN日志显示");
    ESP_LOGI(TAG, "INFO日志不显示");
    ESP_LOGI(TAG_SENSOR, "传感器INFO日志不显示");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "恢复全局日志级别为INFO");
    esp_log_level_set("*", ESP_LOG_INFO);
}

void log_buffer_char_demo(void)
{
    char text[] = "ESP32-S3 Log System Demo";
    ESP_LOGI(TAG, "字符缓冲区转储:");
    ESP_LOG_BUFFER_CHAR(TAG, text, sizeof(text));
}

void log_performance_demo(void)
{
    ESP_LOGI(TAG, "日志性能演示");
    
    int64_t start_time = esp_timer_get_time();
    
    for (int i = 0; i < 100; i++) {
        ESP_LOGD(TAG, "性能测试日志 %d", i);
    }
    
    int64_t end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "100条DEBUG日志耗时: %lld 微秒", end_time - start_time);
}

void log_error_handling_demo(void)
{
    ESP_LOGI(TAG, "错误处理日志演示");
    
    int result = -1;
    if (result != 0) {
        ESP_LOGE(TAG, "操作失败，错误码: %d", result);
    }
    
    result = 0;
    if (result == 0) {
        ESP_LOGI(TAG, "操作成功完成");
    }
}

void log_task_demo(void *pvParameters)
{
    const char *task_tag = (const char *)pvParameters;
    int counter = 0;
    
    while (1) {
        ESP_LOGI(task_tag, "任务运行中，计数器: %d", counter++);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void log_multi_task_demo(void)
{
    ESP_LOGI(TAG, "多任务日志演示");
    
    xTaskCreate(log_task_demo, "LogTask1", 2048, (void *)"TASK1", 5, NULL);
    xTaskCreate(log_task_demo, "LogTask2", 2048, (void *)"TASK2", 5, NULL);
    
    ESP_LOGI(TAG, "创建了两个日志任务");
}

void log_system_info_demo(void)
{
    ESP_LOGI(TAG, "系统信息日志演示");
    
    ESP_LOGI(TAG, "可用堆内存: %d 字节", esp_get_free_heap_size());
    ESP_LOGI(TAG, "最小可用堆内存: %d 字节", esp_get_minimum_free_heap_size());
}

void log_assert_demo(void)
{
    ESP_LOGI(TAG, "断言日志演示");
    
    int value = 10;
    ESP_LOGI(TAG, "检查值是否为正数: %d", value);
    
    if (value > 0) {
        ESP_LOGI(TAG, "断言通过: 值为正数");
    }
}

void log_raw_output_demo(void)
{
    ESP_LOGI(TAG, "原始输出演示");
    
    printf("这是printf原始输出\n");
    ESP_LOGI(TAG, "这是ESP_LOG格式化输出");
    
    ESP_LOGI(TAG, "混合使用:");
    printf("  - 原始输出1\n");
    ESP_LOGI(TAG, "  - ESP_LOG输出");
    printf("  - 原始输出2\n");
}

void app_main(void)
{
    // 设置所有标签的日志级别为VERBOSE，以便看到所有级别的日志
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 日志系统完整示例");
    ESP_LOGI(TAG, "========================================");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "\n--- 1. 日志级别演示 ---");
    log_level_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 2. 日志格式化演示 ---");
    log_format_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 3. 十六进制转储演示 ---");
    log_hex_dump_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 4. 时间戳演示 ---");
    log_timestamp_demo();
    
    ESP_LOGI(TAG, "\n--- 5. 彩色日志演示 ---");
    log_color_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 6. 多标签演示 ---");
    log_tag_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 7. 日志级别控制演示 ---");
    log_level_control_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 8. 全局日志级别演示 ---");
    log_global_level_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 9. 字符缓冲区转储演示 ---");
    log_buffer_char_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 10. 性能演示 ---");
    log_performance_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 11. 错误处理演示 ---");
    log_error_handling_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 12. 系统信息演示 ---");
    log_system_info_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 13. 断言演示 ---");
    log_assert_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 14. 原始输出演示 ---");
    log_raw_output_demo();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "\n--- 15. 多任务日志演示 ---");
    ESP_LOGI(TAG, "注意：这将创建后台任务，需要手动停止");
    log_multi_task_demo();
    
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "所有演示完成！");
    ESP_LOGI(TAG, "========================================");
    
    while (1) {
        ESP_LOGI(TAG, "主程序运行中...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
