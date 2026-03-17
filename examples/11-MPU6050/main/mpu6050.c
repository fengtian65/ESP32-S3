#include "mpu6050.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MPU6050"

// MPU6050 寄存器地址
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_TEMP_OUT_H   0x41
#define MPU6050_REG_GYRO_XOUT_H  0x43

// 灵敏度系数
#define ACCEL_SENSITIVITY_2G    16384.0f
#define ACCEL_SENSITIVITY_4G    8192.0f
#define ACCEL_SENSITIVITY_8G    4096.0f
#define ACCEL_SENSITIVITY_16G   2048.0f

#define GYRO_SENSITIVITY_250    131.0f
#define GYRO_SENSITIVITY_500    65.5f
#define GYRO_SENSITIVITY_1000   32.8f
#define GYRO_SENSITIVITY_2000   16.4f

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t mpu6050_handle = NULL;

// 写入单个寄存器
static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(mpu6050_handle, write_buf, 2, -1);
}

// 读取多个寄存器
static esp_err_t mpu6050_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(mpu6050_handle, &reg, 1, data, len, -1);
}

esp_err_t mpu6050_i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus");
        return ret;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &mpu6050_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MPU6050 device to I2C bus");
        return ret;
    }

    ESP_LOGI(TAG, "I2C master initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_i2c_deinit(void)
{
    esp_err_t ret = ESP_OK;

    if (mpu6050_handle != NULL) {
        ret = i2c_master_bus_rm_device(mpu6050_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove MPU6050 device from I2C bus");
            return ret;
        }
        mpu6050_handle = NULL;
    }

    if (i2c_bus_handle != NULL) {
        ret = i2c_del_master_bus(i2c_bus_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete I2C master bus");
            return ret;
        }
        i2c_bus_handle = NULL;
    }

    ESP_LOGI(TAG, "I2C master deinitialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_init(void)
{
    esp_err_t ret;

    // 唤醒 MPU6050 (清除睡眠位)
    ret = mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU6050");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // 设置采样率分频器 (1kHz / (1 + 0) = 1kHz)
    ret = mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate");
        return ret;
    }

    // 设置低通滤波器
    ret = mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set config");
        return ret;
    }

    // 设置陀螺仪量程为 ±250dps
    ret = mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, GYRO_FS_250DPS << 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set gyro config");
        return ret;
    }

    // 设置加速度计量程为 ±2g
    ret = mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, ACCEL_FS_2G << 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set accel config");
        return ret;
    }

    ESP_LOGI(TAG, "MPU6050 initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_read_accel(float *ax, float *ay, float *az)
{
    uint8_t data[6];
    esp_err_t ret = mpu6050_read_reg(MPU6050_REG_ACCEL_XOUT_H, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_y = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_z = (int16_t)((data[4] << 8) | data[5]);

    *ax = raw_x / ACCEL_SENSITIVITY_2G;
    *ay = raw_y / ACCEL_SENSITIVITY_2G;
    *az = raw_z / ACCEL_SENSITIVITY_2G;

    return ESP_OK;
}

esp_err_t mpu6050_read_gyro(float *gx, float *gy, float *gz)
{
    uint8_t data[6];
    esp_err_t ret = mpu6050_read_reg(MPU6050_REG_GYRO_XOUT_H, data, 6);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_x = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_y = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_z = (int16_t)((data[4] << 8) | data[5]);

    *gx = raw_x / GYRO_SENSITIVITY_250;
    *gy = raw_y / GYRO_SENSITIVITY_250;
    *gz = raw_z / GYRO_SENSITIVITY_250;

    return ESP_OK;
}

esp_err_t mpu6050_read_temp(float *temp)
{
    uint8_t data[2];
    esp_err_t ret = mpu6050_read_reg(MPU6050_REG_TEMP_OUT_H, data, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    int16_t raw_temp = (int16_t)((data[0] << 8) | data[1]);
    *temp = (raw_temp / 340.0f) + 36.53f;

    return ESP_OK;
}
