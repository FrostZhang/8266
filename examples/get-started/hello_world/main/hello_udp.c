#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "esp_log.h"
#include <string.h> //queue.h 依赖

const char *udptag = "udp";

#define CONFIG_EXAMPLE_IPV4 1
#define UDP_HELLO "{\"esp8266\":\"hello\"}"
char *HOST_IP_ADDR = "192.168.43.236";
u16_t PORT = 8266;
int sock = -1;
struct sockaddr_in destAddr = {0};

void udp_client_send(const char *data)
{
        if (sock < 0)
        {
                ESP_LOGE(udptag, "Error occured during sending: sock %d", sock);
                return;
        }
        printf("udp_client_send %s %d %d \n", data, destAddr.sin_addr.s_addr, destAddr.sin_port);
        int err = sendto(sock, data, strlen(data), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0)
        {
                ESP_LOGE(udptag, "Error occured during sending: errno %d", errno);
        }
}

void udp_client_task(void *pvParameters)
{
        // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
        //                     false, true, portMAX_DELAY);

        char rx_buffer[128];
        char addr_str[128];
        int addr_family;
        int ip_protocol;

        while (1)
        {

#ifdef CONFIG_EXAMPLE_IPV4
                destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
                destAddr.sin_family = AF_INET;
                destAddr.sin_port = htons(PORT);
                addr_family = AF_INET;
                ip_protocol = IPPROTO_IP;
                inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
                struct sockaddr_in6 destAddr;
                inet6_aton(HOST_IP_ADDR, &destAddr.sin6_addr);
                destAddr.sin6_family = AF_INET6;
                destAddr.sin6_port = htons(PORT);
                addr_family = AF_INET6;
                ip_protocol = IPPROTO_IPV6;
                inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

                sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
                if (sock < 0)
                {
                        ESP_LOGE(udptag, "Unable to create socket: errno %d", errno);
                        break;
                }
                ESP_LOGI(udptag, "Socket created");
                udp_client_send(UDP_HELLO);
                while (1)
                {

                        struct sockaddr_in sourceAddr; // Large enough for both IPv4 or IPv6
                        socklen_t socklen = sizeof(sourceAddr);
                        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

                        // Error occured during receiving
                        if (len < 0)
                        {
                                ESP_LOGE(udptag, "recvfrom failed: errno %d", errno);
                                break;
                        }
                        // Data received
                        else
                        {
                                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                                ESP_LOGI(udptag, "Received %d bytes from %s:", len, addr_str);
                                ESP_LOGI(udptag, "%s", rx_buffer);
                        }

                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                }

                if (sock != -1)
                {
                        ESP_LOGE(udptag, "Shutting down socket and restarting...");
                        shutdown(sock, 0);
                        close(sock);
                }
        }
        vTaskDelete(NULL);
}
