/*
 * HC-SR04 ultrasonic distance sensor driver (GPIO trigger + echo pulse width).
 *
 * Echo 引脚电平：模块若用 5V 供电，Echo 常为 ~5V；接到 ESP32 GPIO 前请用电阻分压或专用电平转换，避免损坏芯片。
 */

#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 脉宽过短时视为无效回波 (µs) */
#define HC_SR04_MIN_PULSE_US 100

/** @brief 单次测量 Echo 等待超时默认值 (µs)，约对应 >5 m 未收到回波 */
#define HC_SR04_DEFAULT_TIMEOUT_US 30000

typedef struct {
    gpio_num_t trig_gpio;
    gpio_num_t echo_gpio;
} hc_sr04_config_t;

/**
 * @brief 初始化 HC-SR04（配置 Trig 输出、Echo 输入）
 */
esp_err_t hc_sr04_init(const hc_sr04_config_t *cfg);

/**
 * @brief 释放并复位引脚（可选）
 */
void hc_sr04_deinit(void);

/**
 * @brief 测量距离（厘米）
 *
 * @param out_cm      输出距离 (cm)
 * @param timeout_us  Echo 上升沿前最长等待时间
 *
 * @return ESP_OK 成功；
 *         ESP_ERR_TIMEOUT 超时；
 *         ESP_ERR_INVALID_RESPONSE 脉宽异常；
 *         ESP_ERR_INVALID_STATE 未 init
 */
esp_err_t hc_sr04_read_cm(float *out_cm, uint32_t timeout_us);

#ifdef __cplusplus
}
#endif
