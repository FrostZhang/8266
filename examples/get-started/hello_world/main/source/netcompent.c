
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h" //载入资料
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h" //回调
#include "esp_log.h"
#include "esp_event_loop.h" //esp_event_loop_init

#include <stdio.h>
#include <string.h>
//#include <stdlib.h> //queue.h 依赖

#include "netcompent.h"
#include "navcompent.h"
#include "start.h"

static const char *TAG = "netcompent";

static u8_t wifiretry = 0;
static char isSCini = 0;

static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int CONNECTED_FIELD = BIT2;
static const int Net_SUCCESS = BIT3;

net_event_callback_t callback;
ip4_addr_t *LocalIP;

static void smartconfig_callback(smartconfig_status_t status, void *pdata)
{
        switch (status)
        {
        case SC_STATUS_WAIT:
                ESP_LOGI(TAG, "SC_STATUS_WAIT");
                break;
        case SC_STATUS_FIND_CHANNEL:
                ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL  please use airkiss");
                break;
        case SC_STATUS_GETTING_SSID_PSWD:
                ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
                break;
        case SC_STATUS_LINK:
                ESP_LOGI(TAG, "SC_STATUS_LINK");
                wifi_config_t *wifi_config = pdata;
                ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
                ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
                //nvs_handle mHandleNvsRead;
                // //将airkiss获取的wifi写入内存
                // esp_err_t err = nvs_open("wifi", NVS_READWRITE, &mHandleNvsRead);
                // if (err == ESP_OK)
                // {
                //         char ssid[32] = {0};
                //         strcpy(ssid, (char *)wifi_config->sta.ssid);
                //         nvs_set_str(mHandleNvsRead, "ssid", ssid);
                //         char pass[64] = {0};
                //         strcpy(pass, (char *)wifi_config->sta.password);
                //         nvs_set_str(mHandleNvsRead, "pass", pass);
                // }
                // nvs_close(mHandleNvsRead);
                char ssid[32] = {0};
                strcpy(ssid, (char *)wifi_config->sta.ssid);
                char pass[64] = {0};
                strcpy(pass, (char *)wifi_config->sta.password);

                write_wifi(ssid, pass);
                ESP_ERROR_CHECK(esp_wifi_disconnect());
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
                ESP_ERROR_CHECK(esp_wifi_connect());
                break;
        case SC_STATUS_LINK_OVER:
                ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
                if (pdata != NULL)
                {
                        sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
                        switch (sc_callback_data->type)
                        {
                        case SC_ACK_TYPE_ESPTOUCH:
                                ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
                                ESP_LOGI(TAG, "TYPE: ESPTOUCH");
                                break;
                        case SC_ACK_TYPE_AIRKISS:
                                ESP_LOGI(TAG, "TYPE: AIRKISS");
                                break;
                        default:
                                ESP_LOGE(TAG, "TYPE: ERROR");
                                break;
                        }
                }
                xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
                break;
        default:
                break;
        }
}

static void smartconfig_task(void *parm)
{
        EventBits_t uxBits;
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
        ESP_ERROR_CHECK(esp_smartconfig_start(smartconfig_callback));
        int cot = 0;
        isSCini++;
        while (1)
        {
                sys_light((cot++) % 2);
                uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_FIELD | CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, 100); //portMAX_DELAY
                if (uxBits & CONNECTED_BIT)
                {
                        ESP_LOGI(TAG, "WiFi Connected to ap by smartconfig");
                }
                if (uxBits & ESPTOUCH_DONE_BIT)
                {
                        ESP_LOGI(TAG, "smartconfig over");
                        esp_smartconfig_stop();
                        isSCini = 0;
                        sys_light(1);
                        vTaskDelete(NULL);
                }
                else if (uxBits & CONNECTED_FIELD)
                {
                        //配置失败 重新开启airkiss配置
                        esp_smartconfig_stop();
                        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
                        ESP_ERROR_CHECK(esp_smartconfig_start(smartconfig_callback));
                }
                //长时间匹配不上  重启设备
                if (cot>100)
                {
                        ReStart();
                }
                
        }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
        /* For accessing reason codes in case of disconnection */
        system_event_info_t *info = &event->event_info;
        ESP_LOGI(TAG, "wifi handel %d ", event->event_id);
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:
                esp_wifi_connect();
                break;
        case SYSTEM_EVENT_STA_GOT_IP:
                LocalIP = &info->got_ip.ip_info.ip;
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                xEventGroupClearBits(wifi_event_group, CONNECTED_FIELD);
                wifiretry = 0;

                //callback connect
                if (callback != NULL)
                {
                        callback(NET_CONNNECT);
                }

                break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
                ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
                if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT)
                {
                        /*Switch to 802.11 bgn mode */
                        esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
                        esp_wifi_connect();
                        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                }
                else
                {
                        wifiretry++;
                        if (wifiretry < 5)
                        {
                                //断线重连
                                os_delay_us(5000);
                                esp_wifi_connect();
                        }
                        else if (wifiretry == 5)
                        {
                                if (isSCini > 0)
                                {
                                        //airkiss配网失败
                                        xEventGroupSetBits(wifi_event_group, CONNECTED_FIELD);
                                }
                                else //启用airkiss配网
                                        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 1, NULL);
                        }
                        if (callback != NULL)
                        {
                                callback(NET_Disconnect);
                        }
                }
                break;
        default:
                break;
        }
        return ESP_OK;
}

static void initialise_wifi(void)
{
        wifi_config_t wifi_config = {0};
        //navcompent ini wifi custom data
        extern char *wifissid;
        extern char *wifipassword;
        if (wifissid != NULL)
        {
                strncpy((char *)wifi_config.sta.ssid, wifissid, sizeof(wifi_config.sta.ssid));
        }
        else
        {
                ESP_LOGI(TAG, "get str ssid error %s", wifissid);
                strncpy((char *)wifi_config.sta.ssid, CONFIG_ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
                // nvs_set_str(mHandleNvsRead, "ssid", data);
        }

        if (wifipassword != NULL)
        {
                strncpy((char *)wifi_config.sta.password, wifipassword, sizeof(wifi_config.sta.password));
        }
        else
        {
                ESP_LOGI(TAG, "get str password error");
                strncpy((char *)wifi_config.sta.password, CONFIG_ESP_WIFI_SSID, sizeof(wifi_config.sta.password));
                // nvs_set_str(mHandleNvsRead, "ssid", data);
        }

        tcpip_adapter_init();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_LOGI(TAG, "ssid %s   pass %s", wifi_config.sta.ssid, wifi_config.sta.password);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        ESP_ERROR_CHECK(esp_wifi_start());
}

extern void netstart(net_event_callback_t back)
{
        callback = back;
        wifi_event_group = xEventGroupCreate();
        //ESP_ERROR_CHECK(nvs_flash_init());
        initialise_wifi();
}