#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===================== 硬件参数配置 =====================
#define TIMER_FREQ_HZ      1000000  // 定时器时钟频率：1MHz
#define TIMER_PERIOD_MS    1000     // 定时周期：1000毫秒（1秒）
#define LED_PIN            GPIO_NUM_18  // LED引脚
#define GPTIMER_NUM        GPTIMER_NUM_0 // 使用0号GPTimer

// 全局变量：定时器句柄、信号量句柄
gptimer_handle_t gptimer = NULL;
SemaphoreHandle_t timer_sem = NULL;
int led_level = 0; // 本地变量维护电平，不依赖读取引脚

// ===================== 定时器中断回调函数 =====================
bool IRAM_ATTR gptimer_isr_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 检查信号量句柄有效性，释放信号量
    if (timer_sem != NULL) {
        xSemaphoreGiveFromISR(timer_sem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return true;
}

// ===================== LED初始化 =====================
void led_hardware_init(void) {
    gpio_config_t gpio_conf = {
        .pin_bit_mask = 1ULL << LED_PIN,
        .mode = GPIO_MODE_OUTPUT,         // 输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&gpio_conf);
    gpio_set_level(LED_PIN, 0); // 初始熄灭LED
}

// ===================== GPTimer初始化 =====================
void hardware_gptimer_init(void) {
    // 1. 定时器基础配置
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_FREQ_HZ,
        .intr_priority = 0,
        .flags = {
            .intr_shared = false,
            .allow_pd = false,
        }
    };
    gptimer_new_timer(&timer_config, &gptimer);

    // 2. 闹钟配置
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = TIMER_PERIOD_MS * 1000, // 1秒 = 1000ms * 1000us/ms
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);

    // 3. 注册中断回调
    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_isr_callback,
    };
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);

    // 4. 使能并启动定时器
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
}

// ===================== 任务逻辑（翻转LED） =====================
void timer_task_handler(void* arg) {
    led_hardware_init();
    hardware_gptimer_init();

    while (1) {
        // 等待信号量
        if (xSemaphoreTake(timer_sem, portMAX_DELAY) == pdTRUE) {
            // 直接翻转本地变量
            led_level = !led_level;
            // 设置LED电平
            gpio_set_level(LED_PIN, led_level);
            // 打印翻转后的电平
            printf("LED电平翻转：%d\n", led_level);
        }
    }
}

// ===================== 主函数 =====================
void app_main(void) {
    // 创建二进制信号量
    timer_sem = xSemaphoreCreateBinary();

    // 创建任务
    xTaskCreate(
        timer_task_handler,
        "timer_task",
        4096,
        NULL,
        5,
        NULL
    );
}