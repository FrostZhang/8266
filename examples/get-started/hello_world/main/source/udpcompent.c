
#include "lwip/err.h"

#include "udpcompent.h"
#include "datacompent.h"
#include "wificompent.h"

static const char *TAG = "udp";

#define CONFIG_EXAMPLE_IPV4 1
u16_t PORT = 8266;
int sock = -1;
struct sockaddr_in sendAddr = {0};
udp_callback_t callback;
static TaskHandle_t handel;
bool state;
extern void udp_client_bord(const char *data)
{
        if (sock < 0)
        {
                ESP_LOGE(TAG, "Error occured during sending: sock %d", sock);
                return;
        }
        sendAddr.sin_addr.s_addr = IPADDR_BROADCAST;
        printf("udp_client_send %s %d %d \n", data, sendAddr.sin_addr.s_addr, sendAddr.sin_port);
        int err = sendto(sock, data, strlen(data), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
        if (err < 0)
        {
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
        }
}

extern void udp_client_sendto2(const char *ip, const char *data)
{
        if (strlen(ip) > 15)
        {
                ESP_LOGE(TAG, "udp sendto addr err %s", ip);
                return;
        }
        udp_client_sendto(inet_addr(ip), data);
}

extern void udp_client_sendto(in_addr_t addr, const char *data)
{
        if (sock < 0)
                ESP_LOGE(TAG, "udp 没有启用");
        if (addr < 0)
                ESP_LOGE(TAG, "udp sendto addr err");
        sendAddr.sin_addr.s_addr = addr;
        ESP_LOGI(TAG, "udp send %d %s", addr, data);
        int err = sendto(sock, data, strlen(data), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
        if (err < 0)
                ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
}

// #else // IPV6
//                 struct sockaddr_in6 sendAddr;
//                 inet6_aton(HOST_IP_ADDR, &sendAddr.sin6_addr);
//                 sendAddr.sin6_family = AF_INET6;
//                 sendAddr.sin6_port = htons(PORT);
//                 addr_family = AF_INET6;
//                 ip_protocol = IPPROTO_IPV6;
//                 inet6_ntoa_r(sendAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
// #endif

static void udp_client_task(void *pvParameters)
{
        char rx_buffer[256];
        char addr_str[128];
        int addr_family;
        int ip_protocol;
        udp_event event_t;
        while (state)
        {
                sendAddr.sin_addr.s_addr = -1;
                sendAddr.sin_family = AF_INET;
                sendAddr.sin_port = htons(PORT);
                addr_family = AF_INET;
                ip_protocol = IPPROTO_IP;
                inet_ntoa_r(sendAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

                sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
                if (sock < 0)
                {
                        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                        break;
                }
                static struct sockaddr_in mysock = {0};
                mysock.sin_addr.s_addr = LocalIP->addr; //以wifi得到的ip  初始化udp
                mysock.sin_family = AF_INET;
                mysock.sin_port = htons(PORT);
                int err = bind(sock, (struct sockaddr *)&mysock, sizeof(mysock));
                if (err < 0)
                {
                        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
                }
                ESP_LOGI(TAG, "Socket created local ip :%d", mysock.sin_addr.s_addr);
                while (state)
                {
                        printf("udp Stack %ld\n", uxTaskGetStackHighWaterMark(NULL));
                        static struct sockaddr_in reciveAddr; // Large enough for both IPv4 or IPv6
                        socklen_t socklen = sizeof(reciveAddr);
                        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&reciveAddr, &socklen);
                        if (len < 0)
                        {
                                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                                break;
                        }
                        else
                        {
                                inet_ntoa_r(reciveAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
                                rx_buffer[len] = '\0';
                                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                                event_t.addr = reciveAddr.sin_addr.s_addr;
                                event_t.len = len;
                                event_t.recdata = rx_buffer;
                                callback(&event_t);
                                //udp_client_sendto(reciveAddr.sin_addr.s_addr, "get mes");
                        }
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

//开启udp
extern void udp_client_start(udp_callback_t call)
{
        callback = call;
        state = true;
        xTaskCreate(udp_client_task, "udp_client", 1024 * 4, NULL, 5, &handel);
}

extern void udp_client_stop()
{
        if (state)
        {
                state = 0;
        }
}