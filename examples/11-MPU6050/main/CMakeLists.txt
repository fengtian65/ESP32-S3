idf_component_register(SRCS "mpu6050.c" "main.c"
                    REQUIRES driver freertos
                    INCLUDE_DIRS ".")
