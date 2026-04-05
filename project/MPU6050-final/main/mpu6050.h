#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C 配置
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_SDA_IO       8
#define I2C_MASTER_SCL_IO       9
#define I2C_MASTER_FREQ_HZ      400000

// MPU6050 设备地址
#define MPU6050_ADDR            0x68

// 加速度计量程
#define ACCEL_FS_2G             0
#define ACCEL_FS_4G             1
#define ACCEL_FS_8G             2
#define ACCEL_FS_16G            3

// 陀螺仪量程
#define GYRO_FS_250DPS          0
#define GYRO_FS_500DPS          1
#define GYRO_FS_1000DPS         2
#define GYRO_FS_2000DPS         3

// 初始化 I2C 总线
esp_err_t mpu6050_i2c_init(void);

// 初始化 MPU6050
esp_err_t mpu6050_init(void);

// 读取加速度计数据
esp_err_t mpu6050_read_accel(float *ax, float *ay, float *az);

// 读取陀螺仪数据
esp_err_t mpu6050_read_gyro(float *gx, float *gy, float *gz);

// 读取温度数据
esp_err_t mpu6050_read_temp(float *temp);

// 反初始化 I2C 总线
esp_err_t mpu6050_i2c_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // MPU6050_H
