#include "driver/uart.h"
#include "driver/gpio.h"  // 必须添加，否则GPIO_NUM_43/44会报错
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "stdlib.h"  // 必须添加，否则malloc/free会警告

#define UART_PORT_NUM     UART_NUM_0  // 选择UART端口（0/1/2）
#define TXD_PIN           GPIO_NUM_43 // UART TX引脚
#define RXD_PIN           GPIO_NUM_44 // UART RX引脚

#define BUF_SIZE          1024        // 数据缓冲区大小（字节）

// ===================== UART初始化函数 =====================
void uart_init(void) {
    // 1. 定义UART配置结构体
    uart_config_t uart_config = {
        .baud_rate = 115200,                // 波特率：115200（最常用）
        .data_bits = UART_DATA_8_BITS,      // 数据位：8位
        .parity    = UART_PARITY_DISABLE,   // 校验位：无
        .stop_bits = UART_STOP_BITS_1,      // 停止位：1位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // 硬件流控：无
        .source_clk = UART_SCLK_APB,         // 时钟源：APB（80MHz）
    };
    // 2. 应用UART参数配置
    uart_param_config(UART_PORT_NUM, &uart_config);
    // 3. 设置UART引脚
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 4. 安装UART驱动（必须！否则无法读写数据）
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

// ===================== UART数据处理任务 =====================
void uart_task(void *arg) {
    // 1. 分配数据缓冲区内存
    uint8_t* data = (uint8_t*)malloc(BUF_SIZE);
    if (data == NULL) {  // 检查内存分配是否成功
        printf("内存分配失败！\n");
        vTaskDelete(NULL);
        return;
    }

    while(1) {
        // 2. 读取串口数据（阻塞100ms，超时返回0）
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));
        if(len > 0) {
            // 3. 回显数据：把收到的数据发回去
            uart_write_bytes(UART_PORT_NUM, (const char*)data, len);
            // 4. 打印日志：通过UART0打印收到的数据
            printf("收到串口数据：");
            for(int i=0; i<len; i++) {
                printf("%c", data[i]);
            }
            printf("\n");
            // 5. 清空缓冲区：避免下次读取残留数据
            memset(data, 0, BUF_SIZE);
        }
        // 6. 任务延时10ms：释放CPU资源
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 7. 任务退出时释放内存（虽然是无限循环，但规范写法）
    free(data);
    vTaskDelete(NULL);
}

// ===================== 主函数 =====================
void app_main(void) {
    // 1. 初始化UART
    uart_init();
    // 2. 创建UART数据处理任务
    xTaskCreate(
        uart_task,    // 任务函数
        "uart_task",  // 任务名称
        4096,         // 任务栈大小（字节，从2048改为4096，避免栈溢出）
        NULL,         // 任务参数（无）
        10,           // 任务优先级（较高，优先处理串口数据）
        NULL          // 任务句柄（无）
    );
}