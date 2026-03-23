#pragma once

#define MODEL_INPUT_SIZE (100 * 8)
#define MODEL_OUTPUT_SIZE 4

#define INPUT_SCALE 0.03125f
#define INPUT_ZERO_POINT 0

#define OUTPUT_SCALE 0.001953125f
#define OUTPUT_ZERO_POINT 0

extern const uint8_t sensor_cnn_espdl[] asm("_binary_sensor_cnn_espdl_start");
extern const uint8_t sensor_cnn_espdl_end[] asm("_binary_sensor_cnn_espdl_end");
