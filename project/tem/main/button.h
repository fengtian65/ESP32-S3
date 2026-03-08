#ifndef BUTTON_H
#define BUTTON_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief 初始化按键（GPIO11，下降沿触发）
 */
void button_init(void);

/**
 * @brief 获取按键事件队列句柄
 * @return 队列句柄，用于接收按键按下事件（传递 GPIO 编号）
 */
QueueHandle_t button_get_event_queue(void);

#endif // BUTTON_H