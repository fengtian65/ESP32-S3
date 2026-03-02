#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"

#define UART_PORT_NUM     UART_NUM_0
#define TXD_PIN           GPIO_NUM_43
#define RXD_PIN           GPIO_NUM_44

#define BUF_SIZE          1024

void uart_init(void) {
    // UART配置参数
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // 配置UART参数
    uart_param_config(UART_PORT_NUM, &uart_config);
    // 设置引脚
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 安装UART驱动
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void uart_task(void *arg) {
    uint8_t* data = (uint8_t*)malloc(BUF_SIZE);
    while(1) {
        // 读取串口数据
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));
        if(len > 0) {
            // 回显数据
            uart_write_bytes(UART_PORT_NUM, (const char*)data, len);
            // 打印日志
            printf("收到串口数据：");
            for(int i=0; i<len; i++) {
                printf("%c", data[i]);
            }
            printf("\n");
            // 清空缓冲区
            memset(data, 0, BUF_SIZE);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    uart_init();
    // 创建串口处理任务
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);
}