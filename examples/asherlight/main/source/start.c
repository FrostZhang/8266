
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/queue.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "driver/i2c.h"
#include "driver/spi.h"

#include "mqtt_client.h"
//#include "os.h"
#include "dht.h"
#include "ds18b20.h"
#include "driver/adc.h"
#include "driver/hw_timer.h"
#include "udpcompent.h"
#include "sntpcompent.h"
#include "wificompent.h"
#include "datacompent.h"
#include "navcompent.h"
#include "mqttcompent.h"
#include "httpcompent.h"
#include "dscompent.h"
#include "application.h"
#include "otacompent.h"
#include "ledccompent.h"

enum LIGHT_KEY
{
        //红
        R,
        //蓝
        B,
        //绿
        G,
        //亮度
        Bright,
        //波动
        Wave,
};

//0    1  2     3  4  5  12   13 14 15 16
//boot TX light RX io IR LEDC io IR io x
//1 3 在调试状态下 gpio不可用
static const char *TAG = "Main";

//定时器 回调
extern void system_ds_callback(gpio_num_t num, int isopen)
{
        printf("ds callback 回调 %d isopen %d\n", num, isopen);
        cJSON *cj = begin_write_data_bdjs();
        add_write_data_bdjs(cj, KEYS[0], 255);
        add_write_data_bdjs(cj, KEYS[1], 255);
        add_write_data_bdjs(cj, KEYS[2], 255);
        if (isopen)
        {
                int cmds[5] = {255, 255, 255, 255, 0};
                ledc_color2(cmds);
                add_write_data_bdjs(cj, KEYS[3], 255);
        }
        else
        {
                int cmds[5] = {255, 255, 255, 0, 0};
                ledc_color2(cmds);
                add_write_data_bdjs(cj, KEYS[3], 0);
        }
        add_write_data_bdjs(cj, KEYS[4], 0);
        char *send = end_write_data_bdjs();
        mqtt_publish(send);
        data_free(send);
}

//http收到控制信息回调
extern esp_err_t system_http_callback(http_event *call)
{
        if (call->bdjs != NULL)
        {
                data_res *ans = data_decode_bdjs(call->bdjs);
                ledc_color2(ans->cmds);
                cJSON *cj = begin_write_data_bdjs();
                add_write_data_bdjs(cj, KEYS[0], ans->cmds[0]);
                add_write_data_bdjs(cj, KEYS[1], ans->cmds[1]);
                add_write_data_bdjs(cj, KEYS[2], ans->cmds[2]);
                add_write_data_bdjs(cj, KEYS[3], ans->cmds[3]);
                add_write_data_bdjs(cj, KEYS[4], ans->cmds[4]);
                char *send = end_write_data_bdjs();
                mqtt_publish(send);
                data_free(send);
        }
        if (call->restart == 1)
        {
                system_restart();
        }
        return ESP_OK;
}

//mqtt回调
static esp_err_t mqttcallback(char *rec)
{
        //ESP_LOGI(TAG,rec);
        data_res *ans = data_decode_bdjs(rec);
        if (ans == NULL)
        {
                return ESP_OK;
        }
        else
        {
                ledc_color2(ans->cmds);
        }
        return ESP_OK;
}

//sntp 互联网可用? 每秒回调
static esp_err_t sntp_connect_callback(sntp_event *call)
{
        if (call->mestype == SNTP_EVENT_SUCCESS)
        {
                mqtt_app_start(mqttcallback);
        }
        else if (call->mestype == SNTP_EVENT_TIMING)
        {
                if (call->timeinfo->tm_sec == 0)
                {
                        ds_check(call->timeinfo);
                        print_free_heap_size();
                        printf("snt Stack %ld\n", uxTaskGetStackHighWaterMark(NULL));
                }
        }
        else if (call->mestype == SNTP_EVENT_CONNNECTFAILED)
        {
                printf("sntp_connect_failed after 5mis restart");
                // vTaskDelay(5000 / portTICK_RATE_MS);
                // system_restart();
        }
        return ESP_OK;
}

//udp收到信息 现在走的协议是 百度的协议
extern esp_err_t udpcallback(udp_event *call)
{
        ESP_LOGI(TAG, "UDP REC %s", call->recdata);
        printf("UDP Stack %ld\n", uxTaskGetStackHighWaterMark(NULL));
        if (strncmp(call->recdata, "search", 6) == 0)
        {
                char *sysdata = data_get_sysmes();
                udp_client_sendto(call->addr, sysdata);
                free(sysdata);
        }
        else if (strncmp(call->recdata, "{", 1) == 0)
        {
                // data_res *ans = data_decode_bdjs(call->recdata);
                // //接收 其他物理开关 发送的指令
                // if (ans->output0 != NULL)
                // {
                //         printf("udp get reversal %s \n", ans->output0);

                //         //得到 cus_strip 序号
                //         gpio_input_reversal(cus_strip[atoi(ans->output0)]);
                //         char *send = data_bdjs_reported(CMD, gpio_bit);
                //         mqtt_publish(send);
                //         data_free(send);
                // }
        }
        //data_free(data);
        return ESP_OK;
}

//ota 检查结束
static esp_err_t ota_callback_handel()
{
        http_server_start();
        sntp_start(sntp_connect_callback);
        udp_client_start(udpcallback);
        ledc_ini(LEDC_IO_NUM0, LEDC_IO_NUM1, LEDC_IO_NUM2, 2, LEDC_IO_NUM3);
        return ESP_OK;
}

//wifi连接成功 回调
static esp_err_t wifi_callback(net_callback call)
{
        if (call == WIFI_CONNNECT)
        {
                data_initialize();
                ota_check(ota_callback_handel);
        }
        else if (call == WIFI_Disconnect)
        {
                ESP_LOGE(TAG, "收到WiFi断开消息");
                sntpcompent_stop();
                udp_client_stop();
        }
        return ESP_OK;
}

//打印系统
static void print_sys()
{
        /* Print chip information */
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);
        printf("silicon revision %d, ", chip_info.revision);

        printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
               (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        ESP_LOGI(TAG, "Free heap size: %d min size %d\n", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
}

void app_main()
{
        print_sys();
        nav_load_custom_data();
        wifi_connect_start(wifi_callback);
        //vTaskDelay(5000 / portTICK_RATE_MS);

        //int color[3] = {231 * 2048 / 255, 174 * 2048 / 255, 45 * 2048 / 255};
        //ledc_setcolor(color);
}