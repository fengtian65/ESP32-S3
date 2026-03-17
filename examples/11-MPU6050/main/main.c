#include <stdio.h>
#include "esp_log.h"
#include "mpu6050.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MAIN"

void app_main(void)
{
    ESP_LOGI(TAG, "Starting MPU6050 example...");

    // 初始化 I2C
    ESP_ERROR_CHECK(mpu6050_i2c_init());

    // 初始化 MPU6050
    ESP_ERROR_CHECK(mpu6050_init());

    ESP_LOGI(TAG, "MPU6050 setup complete, starting data read loop...");

    float ax, ay, az;
    float gx, gy, gz;
    float temp;

    while (1) {
        // 读取加速度计数据
        if (mpu6050_read_accel(&ax, &ay, &az) == ESP_OK) {
            ESP_LOGI(TAG, "Accel: X=%.3fg, Y=%.3fg, Z=%.3fg", ax, ay, az);
        } else {
            ESP_LOGE(TAG, "Failed to read accelerometer data");
        }

        // 读取陀螺仪数据
        if (mpu6050_read_gyro(&gx, &gy, &gz) == ESP_OK) {
            ESP_LOGI(TAG, "Gyro:  X=%.3fdps, Y=%.3fdps, Z=%.3fdps", gx, gy, gz);
        } else {
            ESP_LOGE(TAG, "Failed to read gyroscope data");
        }

        // 读取温度数据
        if (mpu6050_read_temp(&temp) == ESP_OK) {
            ESP_LOGI(TAG, "Temp:  %.2f°C", temp);
        } else {
            ESP_LOGE(TAG, "Failed to read temperature data");
        }

        ESP_LOGI(TAG, "----------------------------------------");

        // 每秒读取一次
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
