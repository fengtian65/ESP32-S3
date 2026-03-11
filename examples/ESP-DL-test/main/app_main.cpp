#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
// 引入ESP-DL的核心头文件 (这是正确的头文件)
#include "dl_model_base.hpp"
// 引入你的模型数据
#include "model_data.h"

static const char *TAG = "ESP_DL_APP";

// 声明模型输入输出的名称 (如果不知道，可以留空或用索引获取，
// 但通常我们在convert_to_c_array.py时会知道，或者直接用索引操作)
// 这里我们使用更通用的索引方式，避免名字不匹配问题

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP-DL 推理示例开始");

    // ===================== 1. 加载模型 =====================
    // 创建模型对象
    dl::Model *model = new dl::Model();
    
    // 从内存缓冲区加载模型 (model_data 和 model_data_len 来自 model_data.h)
    // 注意：ESP-DL的C++ API通常直接接收 .tflite 数据的指针
    if (!model->load(model_data, model_data_len)) {
        ESP_LOGE(TAG, "模型加载失败！请检查 model_data.h 和模型文件");
        delete model;
        return;
    }
    ESP_LOGI(TAG, "模型加载成功！");

    // ===================== 2. 获取输入输出Tensor =====================
    // 获取输入Tensor (索引0，对应第一个输入)
    dl::Tensor *input = model->get_input(0);
    // 获取输出Tensor (索引0，对应第一个输出)
    dl::Tensor *output = model->get_output(0);

    if (!input || !output) {
        ESP_LOGE(TAG, "获取输入/输出Tensor失败");
        delete model;
        return;
    }

    // 打印模型信息
    ESP_LOGI(TAG, "输入形状: [%d, %d, %d, %d]", 
             input->shape[0], input->shape[1], input->shape[2], input->shape[3]);
    ESP_LOGI(TAG, "输出形状: [%d, %d, %d, %d]", 
             output->shape[0], output->shape[1], output->shape[2], output->shape[3]);

    // ===================== 3. 准备输入数据 =====================
    // 这里用全0作为示例，请替换为你的实际数据
    // 注意：input->data 是 void* 类型，需要根据你的模型量化类型强转 (通常是 int8_t 或 uint8_t)
    // 假设是 int8 量化：
    int8_t *input_data = static_cast<int8_t *>(input->data);
    int input_size = input->get_size(); // 获取元素个数

    // 填充数据 (示例：全0)
    for (int i = 0; i < input_size; i++) {
        input_data[i] = 0; 
    }

    // ===================== 4. 执行推理 =====================
    ESP_LOGI(TAG, "开始推理...");
    model->run(); // 执行前向传播
    ESP_LOGI(TAG, "推理完成！");

    // ===================== 5. 获取结果 =====================
    int8_t *output_data = static_cast<int8_t *>(output->data);
    int output_size = output->get_size();

    ESP_LOGI(TAG, "输出前10个数据:");
    for (int i = 0; i < 10 && i < output_size; i++) {
        printf("%d ", output_data[i]);
    }
    printf("\n");

    // ===================== 6. 清理与循环 =====================
    // 如果你需要循环推理，不要 delete model，把 run() 放到循环里即可
    // delete model; 

    ESP_LOGI(TAG, "示例结束，进入休眠循环");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}