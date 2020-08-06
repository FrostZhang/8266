/* Hello World Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdio.h>
#include <stdlib.h> //queue.h 依赖

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "cJSON.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hello_world_main.h"

#include "nvs_flash.h" //载入资料
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h" //回调
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "tcpip_adapter.h"

#include "driver/i2c.h"
#include "driver/spi.h"

#include "lwip/apps/sntp.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "os.h"
#include "dht.h"
#include "ds18b20.h"

const char *tag = "Hello world";

static xQueueHandle gpio_evt_queue = NULL;
static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int CONNECTED_FIELD = BIT2;
static const int Net_SUCCESS = BIT3;

char isinSC = 0;

static void smartconfig_callback(smartconfig_status_t status, void *pdata)
{
        switch (status)
        {
        case SC_STATUS_WAIT:
                ESP_LOGI(tag, "SC_STATUS_WAIT");
                break;
        case SC_STATUS_FIND_CHANNEL:
                ESP_LOGI(tag, "SC_STATUS_FINDING_CHANNEL  please use airkiss");
                break;
        case SC_STATUS_GETTING_SSID_PSWD:
                ESP_LOGI(tag, "SC_STATUS_GETTING_SSID_PSWD");
                break;
        case SC_STATUS_LINK:
                ESP_LOGI(tag, "SC_STATUS_LINK");
                wifi_config_t *wifi_config = pdata;
                ESP_LOGI(tag, "SSID:%s", wifi_config->sta.ssid);
                ESP_LOGI(tag, "PASSWORD:%s", wifi_config->sta.password);
                nvs_handle mHandleNvsRead;
                esp_err_t err = nvs_open("wifi", NVS_READWRITE, &mHandleNvsRead);
                if (err == ESP_OK)
                {
                        char ssid[32] = {0};
                        strcpy(ssid, (char *)wifi_config->sta.ssid);
                        nvs_set_str(mHandleNvsRead, "ssid", ssid);
                        char pass[64] = {0};
                        strcpy(pass, (char *)wifi_config->sta.password);
                        nvs_set_str(mHandleNvsRead, "pass", pass);
                }
                nvs_close(mHandleNvsRead);
                ESP_ERROR_CHECK(esp_wifi_disconnect());
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
                ESP_ERROR_CHECK(esp_wifi_connect());
                break;
        case SC_STATUS_LINK_OVER:
                ESP_LOGI(tag, "SC_STATUS_LINK_OVER");
                if (pdata != NULL)
                {
                        sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
                        switch (sc_callback_data->type)
                        {
                        case SC_ACK_TYPE_ESPTOUCH:
                                ESP_LOGI(tag, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
                                ESP_LOGI(tag, "TYPE: ESPTOUCH");
                                break;
                        case SC_ACK_TYPE_AIRKISS:
                                ESP_LOGI(tag, "TYPE: AIRKISS");
                                break;
                        default:
                                ESP_LOGE(tag, "TYPE: ERROR");
                                break;
                        }
                }
                xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
                break;
        default:
                break;
        }
}

TaskHandle_t schandle;
void smartconfig_example_task(void *parm)
{
        EventBits_t uxBits;
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
        ESP_ERROR_CHECK(esp_smartconfig_start(smartconfig_callback));
        int cot = 0;
        isinSC = 1;
        while (1)
        {
                gpio_set_level(4, (cot++) % 2);
                uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_FIELD | CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, 100); //portMAX_DELAY
                if (uxBits & CONNECTED_BIT)
                {
                        ESP_LOGI(tag, "WiFi Connected to ap by smartconfig");
                }
                if (uxBits & ESPTOUCH_DONE_BIT)
                {
                        ESP_LOGI(tag, "smartconfig over");
                        esp_smartconfig_stop();
                        isinSC = 0;
                        gpio_set_level(4, 1);
                        vTaskDelete(schandle);
                }
                else if (uxBits & CONNECTED_FIELD)
                {
                        esp_smartconfig_stop();
                        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
                        ESP_ERROR_CHECK(esp_smartconfig_start(smartconfig_callback));
                }
        }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
        /* For accessing reason codes in case of disconnection */
        system_event_info_t *info = &event->event_info;
        ESP_LOGI(tag, "wifi handel %d ", event->event_id);
        switch (event->event_id)
        {
        case SYSTEM_EVENT_STA_START:
                esp_wifi_connect();
                break;
        case SYSTEM_EVENT_STA_GOT_IP:
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                xEventGroupClearBits(wifi_event_group, CONNECTED_FIELD);
                //发布消息给smc sntp
                break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
                ESP_LOGE(tag, "Disconnect reason : %d", info->disconnected.reason);
                if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT)
                {
                        /*Switch to 802.11 bgn mode */
                        esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
                        esp_wifi_connect();
                        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                }
                else
                {
                        if (isinSC == 1)
                        {
                                xEventGroupSetBits(wifi_event_group, CONNECTED_FIELD);
                        }
                        else
                                xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, &schandle, 1, NULL);
                }
                break;
        default:
                break;
        }
        return ESP_OK;
}

static void initialise_wifi(void)
{
        //NVS操作的句柄，类似于 rtos系统的任务创建返回的句柄！
        nvs_handle mHandleNvsRead;
        int8_t nvs_i8 = 0;

        esp_err_t err = nvs_open("wifi", NVS_READWRITE, &mHandleNvsRead);
        //打开数据库，打开一个数据库就相当于会返回一个句柄
        if (err != ESP_OK)
        {
                ESP_LOGE(tag, "Open NVS Table fail");
                vTaskDelete(NULL);
        }
        else
        {
                ESP_LOGI(tag, "Open NVS Table ok.");
        }

        //读取 字符串
        char ssid[32] = {0};
        uint32_t len = sizeof(ssid);
        err = nvs_get_str(mHandleNvsRead, "ssid", ssid, &len);
        wifi_config_t wifi_config = {0};
        if (err == ESP_OK)
        {
                ESP_LOGI(tag, "get str ssid = %s ", ssid);
                strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        }
        else
        {
                ESP_LOGI(tag, "get str ssid error");
                strncpy((char *)wifi_config.sta.ssid, CONFIG_ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
                // nvs_set_str(mHandleNvsRead, "ssid", data);
        }

        char pass[64] = {0};
        len = sizeof(pass);
        err = nvs_get_str(mHandleNvsRead, "pass", pass, &len);

        if (err == ESP_OK)
        {
                ESP_LOGI(tag, "get str pass = %s ", pass);
                strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        }
        else
        {
                ESP_LOGI(tag, "get str pass error");
                strncpy((char *)wifi_config.sta.password, CONFIG_ESP_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
                // nvs_set_str(mHandleNvsRead, "ssid", data);
        }

        //关闭数据库，关闭面板！
        nvs_close(mHandleNvsRead);

        tcpip_adapter_init();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        //ESP_ERROR_CHECK( esp_wifi_start() );
        // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        //ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        // wifi_config_t wifi_config = {
        //     .sta = {
        //         .ssid = &ssid,
        //         .password = &pass,
        //     },
        // };

        ESP_LOGI(tag, "ssid %s   pass %s", ssid, pass);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        ESP_ERROR_CHECK(esp_wifi_start());
}

static void initialize_sntp(void)
{
        ESP_LOGI(tag, "Initializing SNTP");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();
}

static void obtain_time(void)
{
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);

        initialize_sntp();

        // wait for time to be set
        time_t now = 0;
        struct tm timeinfo = {0};
        int retry = 0;
        const int retry_count = 10;

        while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
        {
                ESP_LOGI(tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                time(&now);
                localtime_r(&now, &timeinfo);
        }
}

static void sntp_example_task(void *arg)
{
        time_t now;
        struct tm timeinfo;
        char strftime_buf[64];

        time(&now);
        localtime_r(&now, &timeinfo);

        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900))
        {
                ESP_LOGI(tag, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
                obtain_time();
        }

        // Set timezone to Eastern Standard Time and print local time
        //setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
        //tzset();

        // Set timezone to China Standard Time
        setenv("TZ", "CST-8", 1);
        tzset();

        while (1)
        {
                // update 'now' variable with current time
                time(&now);
                localtime_r(&now, &timeinfo);

                if (timeinfo.tm_year < (2016 - 1900))
                {
                        ESP_LOGE(tag, "The current date/time error");
                }
                else
                {
                        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
                        ESP_LOGI(tag, "The current date/time in Shanghai is: %s", strftime_buf);
                        ESP_LOGI(tag, "Free heap size: %d\n", esp_get_free_heap_size());
                        xEventGroupSetBits(wifi_event_group, Net_SUCCESS);
                        vTaskDelete(NULL);
                }
                vTaskDelay(1000 / portTICK_RATE_MS);
        }
}

static void TestJson(void)
{
        printf(cJSON_Version());
        //json send
        cJSON *jsonSend = cJSON_CreateObject();
        cJSON_AddItemToObject(jsonSend, "name", cJSON_CreateString("esp8266"));
        cJSON *jsonSendprop = cJSON_CreateObject();
        cJSON_AddItemToObject(jsonSend, "prop", jsonSendprop);
        cJSON_AddItemToObject(jsonSendprop, "ip", cJSON_CreateString("192.168.1.1"));
        cJSON_AddItemToObject(jsonSendprop, "res", cJSON_CreateTrue());

        char *sendstr = cJSON_Print(jsonSend);
        if (NULL != sendstr)
        {
                ESP_LOGI(tag, sendstr);
                free(sendstr);
        }
}

static void gpio_isr_handler(void *arg)
{
        uint32_t gpio_num = (uint32_t)arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
        uint32_t io_num;
        for (;;)
        {
                if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
                {
                        ESP_LOGI(tag, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
                }
        }
}

static void GpioIni(void)
{
        //灯显
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = GPIO_Pin_4;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_NUM_4, 1);

        //boot press
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = GPIO_Pin_0;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);

        gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        xTaskCreate(gpio_task_example, "gpio_task_example", 1024, NULL, 10, NULL);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *)GPIO_NUM_0);
}

static void SmartConfig(void)
{
        //SmartConfig
        ESP_ERROR_CHECK(nvs_flash_init());
        initialise_wifi();
}

static void ReStart()
{
        //设备重启
        printf("Restarting now.\n");
        fflush(stdout);
        esp_restart();
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
        esp_mqtt_client_handle_t client = event->client;
        int msg_id;
        // your_context_t *context = event->context;
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
                ESP_LOGI(tag, "MQTT_EVENT_CONNECTED");
                esp_mqtt_client_subscribe(client, "$baidu/iot/shadow/Asher8266/update/accepted", 0);
                esp_mqtt_client_subscribe(client, "$baidu/iot/shadow/Asher8266/get/accepted", 0);
                break;
        case MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(tag, "MQTT_EVENT_DISCONNECTED");
                break;
        case MQTT_EVENT_SUBSCRIBED:
                esp_mqtt_client_publish(client, "$baidu/iot/shadow/Asher8266/get", "{}", 0, 0, 0);
                break;
        case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_PUBLISHED:
                ESP_LOGI(tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_DATA:
                printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                // printf("DATA=%.*s\r\n", event->data_len, event->data);
                cJSON *json = cJSON_Parse(event->data);
                if (NULL != json)
                {
                        cJSON *reported = cJSON_GetObjectItem(json, "reported");
                        if (NULL != reported)
                        {
                                cJSON *Switch = cJSON_GetObjectItem(reported, "Switch");
                                if (cJSON_IsNumber(Switch))
                                {
                                        gpio_set_level(4, (Switch->valueint + 1) % 2);
                                }
                        }
                        cJSON_Delete(json);
                }
                break;
        case MQTT_EVENT_ERROR:
                ESP_LOGI(tag, "MQTT_EVENT_ERROR");
                break;
        }
        return ESP_OK;
}

static void mqtt_app_start(void)
{
        xEventGroupWaitBits(wifi_event_group, Net_SUCCESS,
                            true, true, portMAX_DELAY);
        const esp_mqtt_client_config_t mqtt_cfg = {
            .event_handle = mqtt_event_handler,
            .host = "t0eff28.mqtt.iot.gz.baidubce.com",
            .port = 1883,
            .username = "t0eff28/Asher8266",
            .password = "is7vmgfjer4e7uq9",
            // .user_context = (void *)your_context
        };

        esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_start(client);

        //不保持会被回收 报指针为null
        while (1)
        {
                vTaskDelay(1000 / portTICK_RATE_MS);
        }
}

void Taskds18b20(void *p)
{
        ESP_ERROR_CHECK(Ds18b20Init());
        while (1)
        {
                printf("ds18b20采集的温度: %d \n\n", (int)(Ds18b20ReadTemp() * 0.0625 + 0.005));
                vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
}

void TaskCreatDht11(void *p)
{
        uint8_t curTem = 0;
        uint8_t curHum = 0;
        ESP_LOGI(tag, " dh11Init() : %d ", dh11Init());
        while (1)
        {
                vTaskDelay(5000 / portTICK_RATE_MS);
                dh11Read(&curTem, &curHum);
                //ESP_LOGI(tag, "Temperature : %d , Humidity : %d", curTem, curHum);
        }
        vTaskDelete(NULL);
}

void app_main()
{
        printf("Hello world!\n");

        /* Print chip information */
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

        printf("silicon revision %d, ", chip_info.revision);

        printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
               (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

        //TestJson();
        GpioIni();
        wifi_event_group = xEventGroupCreate();
        //OLED 未完成
        // OLED_Init();
        // OLED_ShowString(0, 0, "Project=");
        // OLED_ShowString(64,0,"IIC_OLED");
        SmartConfig();

        xTaskCreate(sntp_example_task, "sntp_example_task", 2048, NULL, 8, NULL);

        long ret = xTaskCreate(mqtt_app_start, "mqtt_client", 4096, NULL, 10, NULL);
        if (ret != pdPASS)
        {
                ESP_LOGE(tag, "mqtt create client thread failed");
        }

        xTaskCreate(TaskCreatDht11, "TaskCreatDht11", 2048, NULL, 4, NULL);
        //xTaskCreate(Taskds18b20, "Taskds18b20", 2048, NULL, 3, NULL);
}
