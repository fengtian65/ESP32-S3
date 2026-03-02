#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===================== PWM配置宏定义 =====================
#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_LOW_SPEED_MODE  // ESP32-S3仅支持低速模式
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH0_GPIO       18 // LED引脚
#define LEDC_DUTY_RES          LEDC_TIMER_10_BIT // 占空比分辨率：0-1023
#define LEDC_FREQUENCY         5000 // PWM频率：5000Hz

// ===================== PWM初始化函数 =====================
void ledc_pwm_init(void) {
    // 1. 配置定时器
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_HS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // 2. 配置通道
    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_HS_CH0_CHANNEL,
        .duty       = 0, // 初始占空比：0（LED灭）
        .gpio_num   = LEDC_HS_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER
    };
    ledc_channel_config(&ledc_channel);
}

// ===================== PWM LED渐变任务函数 =====================
void pwm_led_task(void *arg) {
    // 初始化PWM
    ledc_pwm_init();
    
    int duty = 0;
    int step = 5; // 占空比步长

    // 任务无限循环
    while(1) {
        // 设置占空比（0-1023）
        ledc_set_duty(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL, duty);
        ledc_update_duty(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL); // 必须调用，占空比才生效
        
        // 渐变逻辑：递增到最大值后递减
        duty += step;
        if(duty >= 1023 || duty <= 0) {
            step = -step;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 渐变速度
    }
}

// ===================== 主函数（仅保留任务创建） =====================
void app_main(void) {
    // 创建PWM LED渐变任务
    xTaskCreate(
        pwm_led_task,    // 任务函数
        "pwm_led_task",  // 任务名称
        4096,            // 任务栈大小（字节，PWM操作足够）
        NULL,            // 任务参数（无）
        2,               // 任务优先级（普通优先级）
        NULL             // 任务句柄（无）
    );
}