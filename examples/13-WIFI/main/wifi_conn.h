#ifndef WIFI_CONN_H_
#define WIFI_CONN_H_

#include "esp_types.h"
#include <stdbool.h>
#include "esp_err.h"
#include "stdint.h"
#include "esp_wifi_types.h"

#define WIFI_SSID      "zhangfanding"
#define WIFI_PASS      "12345678"
#define MAX_SCAN_RESULTS 10

typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_ap_info_t;

void wifi_conn_init(void);
bool wifi_is_connected(void);
void wifi_get_ip(char *ip_buf, size_t buf_size);
int8_t wifi_get_rssi(void);
esp_err_t wifi_scan_ap(wifi_ap_info_t *ap_list, uint16_t *ap_count);
esp_err_t wifi_connect_to_ap(const char *ssid, const char *password);
void wifi_disconnect(void);
void wifi_reconnect(void);
void wifi_get_ssid(char *ssid_buf, size_t buf_size);

#endif
