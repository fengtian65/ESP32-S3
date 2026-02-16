idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES driver freertos esp_wifi esp_event nvs_flash
                    INCLUDE_DIRS ".")

