
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/err.h"

#include "udpcompent.h"
#include "datacompent.h"

static const char *TAG = "udp";

#define CONFIG_EXAMPLE_IPV4 1
static const char *HOST_IP_ADDR = "255.255.255.255";
u16_t PORT = 8266;
int sock = -1;
struct sockaddr_in destAddr = {0};
udp_callback_t callback;

extern void udp_client_send(const char *data)
{
        if (sock < 0)
        {
                ESP_LOGE(TAG, "Error occured during sending: sock %d", sock);
                return;
        }
        printf("udp_client_send %s %d %d \n", data, destAddr.sin_addr.s_addr, destAddr.sin_port);
        int err = sendto(sock, data, strlen(data), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0)
        {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
        }
}

static void udp_client_task(void *pvParameters)
{
        char rx_buffer[256];
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
                        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                        break;
                }
                ESP_LOGI(TAG, "Socket created");
                while (1)
                {

                        struct sockaddr_in sourceAddr; // Large enough for both IPv4 or IPv6
                        socklen_t socklen = sizeof(sourceAddr);
                        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

                        //// Error occured during receiving
                        // if (len < 0)
                        // {
                        //         ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                        //         break;
                        // }
                        // // Data received
                        // else
                        // {
                        //         rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                        //         ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                        //         ESP_LOGI(TAG, "%s", rx_buffer);
                        // }
                        callback(rx_buffer,len);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                }

                if (sock != -1)
                {
                        ESP_LOGE(TAG, "Shutting down socket and restarting...");
                        shutdown(sock, 0);
                        close(sock);
                }
        }
        vTaskDelete(NULL);
}
 
extern void udp_client_start(udp_callback_t call)
{
        callback = call;
        xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
}