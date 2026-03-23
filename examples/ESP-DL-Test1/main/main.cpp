#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "dl_model_base.hpp"
#include "dl_tensor_base.hpp"
#include "fbs_loader.hpp"
#include "model_define.h"

using namespace dl;

static const char *TAG = "ESP-DL-Test";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP-DL Model Loading Test");
    ESP_LOGI(TAG, "=========================");

    size_t model_size = sensor_cnn_espdl_end - sensor_cnn_espdl;
    ESP_LOGI(TAG, "Model size: %zu bytes", model_size);
    ESP_LOGI(TAG, "Model address: 0x%08lx", (uintptr_t)sensor_cnn_espdl);
    ESP_LOGI(TAG, "Model address aligned: %s", ((uintptr_t)sensor_cnn_espdl & 0xF) ? "No" : "Yes");

    ESP_LOGI(TAG, "First 16 bytes of model:");
    ESP_LOG_BUFFER_HEX(TAG, sensor_cnn_espdl, 16);

    // 检查模型格式
    char format[5];
    memcpy(format, sensor_cnn_espdl, 4);
    format[4] = '\0';
    ESP_LOGI(TAG, "Model format: %s", format);

    // 检查 ESP-DL 版本
    ESP_LOGI(TAG, "\nESP-DL Version: 3.2.4");
    ESP_LOGI(TAG, "Commit: 7089b94a76206825bddba57a6385d46cc08c0a6b");

    // 强制设置 param_copy = true，避免直接访问 FLASH/PSRAM
    bool param_copy = true;
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "PSRAM size: %zu bytes", psram_size);
    ESP_LOGI(TAG, "Forcing param_copy=true to avoid direct FLASH/PSRAM access");

    ESP_LOGI(TAG, "\nTest: Creating Model directly...");
    Model *model = nullptr;
    
    // 使用官方示例的方法：直接使用 Model 构造函数
    model = new Model((const char *)sensor_cnn_espdl, 
                      fbs::MODEL_LOCATION_IN_FLASH_RODATA, 
                      0, 
                      dl::MEMORY_MANAGER_GREEDY, 
                      nullptr, 
                      param_copy);
    
    if (model == nullptr) {
        ESP_LOGE(TAG, "Model is null");
        return;
    }
    ESP_LOGI(TAG, "Model created successfully");

    ESP_LOGI(TAG, "\nTest: Getting inputs...");
    auto inputs = model->get_inputs();
    ESP_LOGI(TAG, "Number of inputs: %d", inputs.size());

    ESP_LOGI(TAG, "\nTest: Getting outputs...");
    auto outputs = model->get_outputs();
    ESP_LOGI(TAG, "Number of outputs: %d", outputs.size());

    ESP_LOGI(TAG, "\n=========================");
    ESP_LOGI(TAG, "All tests completed!");
    ESP_LOGI(TAG, "=========================");

    // Cleanup
    delete model;
    ESP_LOGI(TAG, "Resources released");
}
