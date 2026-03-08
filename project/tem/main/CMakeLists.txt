idf_component_register(SRCS "wifi_conn.c" "dht11.c" "main.c"
                       INCLUDE_DIRS ""
                       REQUIRES esp_wifi esp_event nvs_flash esp_http_client json driver esp_timer)