#ifndef DHT11_H_
#define DHT11_H_

#include "driver/gpio.h"
#include "esp_err.h"

/* ==================== DHT11配置项 ==================== */
#define DHT11_GPIO     GPIO_NUM_5  // DHT11数据引脚，可在此修改

/* ==================== 函数声明 ==================== */
/**
 * @brief 初始化DHT11驱动
 */
void dht11_init(void);

/**
 * @brief 读取DHT11的温度和湿度
 * @param temp 温度输出指针（℃）
 * @param humid 湿度输出指针（%）
 * @return 0-成功，-1-失败
 */
int dht11_read(float *temp, float *humid);

#endif // DHT11_H_