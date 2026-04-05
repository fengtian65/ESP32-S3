idf_component_register(SRCS "mpu6050.c" "main.c"
                    REQUIRES driver freertos esp_wifi esp_event nvs_flash lwip
                    INCLUDE_DIRS ".")
