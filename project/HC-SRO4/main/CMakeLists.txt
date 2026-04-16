idf_component_register(SRCS "main.c" "hc_sr04.c" "mpu6050.c"
                    REQUIRES driver freertos esp_timer
                    INCLUDE_DIRS ".")
