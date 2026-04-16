#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "hc_sr04.h"
#include "mpu6050.h"

/* 仅输出 MPU6050 数据；其它组件 INFO 关闭 */
static const char *TAG_MPU = "MPU6050";

#define HC_SR04_TRIG_GPIO GPIO_NUM_4
#define HC_SR04_ECHO_GPIO GPIO_NUM_5

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG_MPU, ESP_LOG_INFO);

    esp_err_t err = mpu6050_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MPU, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }
    err = mpu6050_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MPU, "MPU6050 init failed: %s", esp_err_to_name(err));
        return;
    }

    hc_sr04_config_t sr04 = {
        .trig_gpio = HC_SR04_TRIG_GPIO,
        .echo_gpio = HC_SR04_ECHO_GPIO,
    };
    (void)hc_sr04_init(&sr04);

    float ax, ay, az, gx, gy, gz, temp;

    while (1) {
        if (mpu6050_read_accel(&ax, &ay, &az) != ESP_OK) {
            continue;
        }
        if (mpu6050_read_gyro(&gx, &gy, &gz) != ESP_OK) {
            continue;
        }
        if (mpu6050_read_temp(&temp) != ESP_OK) {
            continue;
        }

        ESP_LOGI(TAG_MPU,
                 "accel g: %.3f %.3f %.3f | gyro dps: %.2f %.2f %.2f | temp C: %.1f",
                 ax, ay, az, gx, gy, gz, temp);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
