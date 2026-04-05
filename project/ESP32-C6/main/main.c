#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#define PORT 8888
#define MAX_CONNECTIONS 1
#define BUFFER_SIZE 1024

static const char *TAG = "TCP_SERVER";

void tcp_server_task(void *pvParameters)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char recv_buffer[BUFFER_SIZE] = {0};
    int recv_len = 0;

    // 创建socket文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        ESP_LOGE(TAG, "socket creation failed");
        vTaskDelete(NULL);
    }

    // 设置socket选项
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        ESP_LOGE(TAG, "setsockopt failed");
        close(server_fd);
        vTaskDelete(NULL);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定socket到端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        ESP_LOGE(TAG, "bind failed");
        close(server_fd);
        vTaskDelete(NULL);
    }

    // 开始监听
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        ESP_LOGE(TAG, "listen failed");
        close(server_fd);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Server listening on port %d", PORT);

    while (1)
    {
        ESP_LOGI(TAG, "Waiting for connection...");
        // 接受连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            ESP_LOGE(TAG, "accept failed");
            continue;
        }

        ESP_LOGI(TAG, "Connected by %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // 接收数据
        while (1)
        {
            recv_len = recv(new_socket, recv_buffer, BUFFER_SIZE - 1, 0);
            if (recv_len < 0)
            {
                ESP_LOGE(TAG, "recv failed");
                break;
            }
            else if (recv_len == 0)
            {
                ESP_LOGI(TAG, "Client disconnected");
                break;
            }

            recv_buffer[recv_len] = '\0';
            
            // 获取时间戳
            time_t now = time(NULL);
            struct tm *timeinfo = localtime(&now);
            char timestamp[30];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

            // 处理接收到的数据
            char *line = strtok(recv_buffer, "\n");
            while (line != NULL)
            {
                // 解析数据
                float ax, ay, az, gx, gy, gz, temp;
                if (sscanf(line, "%f,%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &gx, &gy, &gz, &temp) == 7)
                {
                    ESP_LOGI(TAG, "[%s] Accel: %.3f, %.3f, %.3f | Gyro: %.3f, %.3f, %.3f | Temp: %.2f°C",
                             timestamp, ax, ay, az, gx, gy, gz, temp);
                }
                else
                {
                    ESP_LOGE(TAG, "[%s] Failed to parse data: %s", timestamp, line);
                }
                line = strtok(NULL, "\n");
            }
        }

        close(new_socket);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting TCP server...");
    xTaskCreate(tcp_server_task, "tcp_server_task", 4096, NULL, 5, NULL);
}
