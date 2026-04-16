#include "hc_sr04.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

/* 声速近似换算：往返时间(µs)/58 ≈ 单程距离(cm)，与模块常用手册一致 */
#define PULSE_US_TO_CM_NUM 58.0f

/* 最大合理脉宽约对应 ~4 m 量程 (µs) */
#define HC_SR04_MAX_PULSE_US 25000

static gpio_num_t s_trig = GPIO_NUM_NC;
static gpio_num_t s_echo = GPIO_NUM_NC;
static bool s_inited;

esp_err_t hc_sr04_init(const hc_sr04_config_t *cfg)
{
    if (cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!GPIO_IS_VALID_GPIO(cfg->trig_gpio) || !GPIO_IS_VALID_GPIO(cfg->echo_gpio)) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << cfg->trig_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&trig_conf);
    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << cfg->echo_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&echo_conf);
    if (err != ESP_OK) {
        return err;
    }

    gpio_set_level(cfg->trig_gpio, 0);

    s_trig = cfg->trig_gpio;
    s_echo = cfg->echo_gpio;
    s_inited = true;
    return ESP_OK;
}

void hc_sr04_deinit(void)
{
    if (!s_inited) {
        return;
    }
    gpio_reset_pin(s_trig);
    gpio_reset_pin(s_echo);
    s_trig = GPIO_NUM_NC;
    s_echo = GPIO_NUM_NC;
    s_inited = false;
}

esp_err_t hc_sr04_read_cm(float *out_cm, uint32_t timeout_us)
{
    if (out_cm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(s_trig, 0);
    esp_rom_delay_us(2);
    gpio_set_level(s_trig, 1);
    esp_rom_delay_us(10);
    gpio_set_level(s_trig, 0);

    const int64_t wait0 = esp_timer_get_time();
    while (gpio_get_level(s_echo) == 0) {
        if ((esp_timer_get_time() - wait0) > (int64_t)timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
    }

    const int64_t rise = esp_timer_get_time();
    while (gpio_get_level(s_echo) == 1) {
        if ((esp_timer_get_time() - rise) > HC_SR04_MAX_PULSE_US) {
            return ESP_ERR_TIMEOUT;
        }
    }
    const int64_t fall = esp_timer_get_time();

    const int64_t pulse = fall - rise;
    if (pulse < HC_SR04_MIN_PULSE_US || pulse > HC_SR04_MAX_PULSE_US) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    *out_cm = (float)pulse / PULSE_US_TO_CM_NUM;
    return ESP_OK;
}
