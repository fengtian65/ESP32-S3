#ifndef WIFI_CONN_H_
#define WIFI_CONN_H_

// 第一步：优先引入ESP官方类型头文件（必加，适配ESP32环境）
#include "esp_types.h"
// 备用：如果esp_types.h仍不生效，直接加stdbool.h（放在最顶部）
#include <stdbool.h>

#include "esp_err.h"
#include "stdint.h"

/* ==================== WiFi配置项 ==================== */
#define WIFI_SSID      "zhangfanding"  // WiFi名称，可在此修改
#define WIFI_PASS      "12345678"      // WiFi密码，可在此修改

/* ==================== 函数声明 ==================== */
/**
 * @brief 初始化WiFi连接（STA模式）
 */
void wifi_conn_init(void);

/**
 * @brief 获取WiFi连接状态
 * @return true-已连接，false-未连接
 */
bool wifi_is_connected(void);

#endif // WIFI_CONN_H_