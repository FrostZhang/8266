
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/queue.h"
#include "esp_spi_flash.h"

#include "mqtt_client.h"

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

//0    1  2     3  4  5  12   13 14 15 16
//boot TX light RX io IR LEDC io IR io x
//1 3 在调试状态下 gpio不可用
static const char *TAG = "Main";
static xQueueHandle gpio_evt_queue = NULL;

static int gpio_input_reversal(gpio_num_t num);
static void gpio_input_num(gpio_num_t num, int level);
//缓冲 防止物理开关接触不良
static int isr_level_temp[] = {0, 0, 0, 0};

static void gpio_isr_handler(void *arg)
{
        uint32_t gpio_num = (uint32_t)arg;
        int level = gpio_get_level(gpio_num);
        os_delay_us(60000);
        if (gpio_get_level(gpio_num) != level)
                return;
        for (uint8_t i = 0; i < sizeof(cus_isr); i++)
        {
                if (cus_isr[i] == gpio_num)
                {
                        if (level == isr_level_temp[i])
                        {
                                return;
                        }
                        isr_level_temp[i] = level;
                }
        }
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task(void *arg)
{
        uint32_t io_num = (uint32_t)arg;
        for (;;)
        {
                if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
                {
                        for (uint8_t i = 0; i < sizeof(cus_isr); i++)
                        {
                                if (cus_isr[i] == io_num)
                                {
                                        printf("GPIO[%d] intr\n", io_num);
                                        if (strlen(isr_events[i].ip) < 9)
                                        {
                                                int res = gpio_input_reversal(cus_strip[isr_events[i].for_strip_index]);
                                                char *send = data_bdjs_reported(KEYS[i], res);
                                                mqtt_publish(send);
                                                data_free(send);
                                        }
                                        else
                                        {
                                                char string[5] = {0};
                                                char *send = data_bdjs_reported_string(OUTPUT0, itoa(isr_events[i].for_strip_index, string, 10));
                                                udp_client_sendto2(isr_events[i].ip, send);
                                                data_free(send);
                                        }
                                }
                        }
                }
        }
        ESP_LOGI(TAG, "gpio_task 将停止工作");
        vTaskDelete(NULL);
}

//初始化gpio
static void GpioIni(void)
{
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE; //取消中断
        io_conf.mode = GPIO_MODE_OUTPUT;       //对外输出 控制设备
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        io_conf.pin_bit_mask = 0;
        for (uint8_t i = 0; i < sizeof(cus_strip); i++)
                io_conf.pin_bit_mask |= BIT(cus_strip[i]);
        gpio_config(&io_conf);
        os_delay_us(20); //延时20MS 等配置完毕

        for (uint8_t i = 0; i < sizeof(cus_strip); i++)
                gpio_set_level(cus_strip[i], 0);
        io_conf.pin_bit_mask = GPIO_Pin_2;
        gpio_config(&io_conf);
        gpio_set_level(GPIO_NUM_2, OFF); //
        // button_handle_t btn_handle = iot_button_create(GPIO_NUM_0, BUTTON_ACTIVE_LOW);
        // iot_button_add_custom_cb(btn_handle, 5, button_press_5s_cb, NULL);
        //boot press
        //gpio0  gpio_set_level1 即使控制引脚输出了高电平,当按下按钮的时候,引脚接地,引脚强制被拉低.
        //PIN_FUNC_SELECT
        //别人写的是 input pull up
        io_conf.mode = GPIO_MODE_OUTPUT_OD;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        io_conf.pin_bit_mask = 0;
        for (uint8_t i = 0; i < 4; i++)
                io_conf.pin_bit_mask |= BIT(cus_isr[i]);
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf);
        os_delay_us(20); //延时20MS 等配置完毕

        gpio_evt_queue = xQueueCreate(16, sizeof(uint32_t));
        xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 1, NULL);
        gpio_install_isr_service(0);
        for (uint8_t i = 0; i < 4; i++)
        {
                uint8_t n = i;
                gpio_isr_handler_add(cus_isr[n], gpio_isr_handler, (void *)cus_isr[n]);
        }
}

static int included_cus_strip(gpio_num_t num)
{
        for (uint8_t i = 0; i < 4; i++)
        {
                if (num == cus_strip[i])
                {
                        return i;
                }
        }
        return -1;
}

//控制某一个 cus_strip 的状态
static void gpio_input_num(gpio_num_t num, int level)
{
        if (level != 0 && level != 1)
                return;
        if (included_cus_strip(num) > -1)
        {
                gpio_set_level(num, level);
        }
        else
        {
                ESP_LOGE(TAG, "%d 不在自定义控制列表", num);
        }
}

//反转某一个 cus_strip 的状态
static int gpio_input_reversal(gpio_num_t num)
{
        int set = -1;
        if (included_cus_strip(num) > -1)
        {
                if (system_get_gpio_state(num))
                {
                        gpio_set_level(num, ON);
                        set = ON;
                }
                else
                {
                        gpio_set_level(num, OFF);
                        set = OFF;
                }
        }
        else
        {
                ESP_LOGE(TAG, "%d 不在自定义控制列表", num);
        }
        return set;
}

//定时器 回调
extern void system_ds_callback(gpio_num_t num, int isopen)
{
        printf("ds callback 回调 %d isopen %d\n", num, isopen);
        int index = included_cus_strip(num);
        if (index > -1)
        {
                gpio_input_num(num, isopen);
                char *send = data_bdjs_reported(KEYS[index], isopen);
                mqtt_publish(send);
                data_free(send);
        }
}

//http收到控制信息回调
extern esp_err_t system_http_callback(http_event *call)
{
        if (call->bdjs != NULL)
        {
                data_res *ans = data_decode_bdjs(call->bdjs);
                for (uint8_t i = 0; i < 4; i++)
                {
                        if (ans->cmds[i] != -2)
                        {
                                gpio_input_num(cus_strip[i], ans->cmds[i]);
                                char *send = data_bdjs_reported(KEYS[i], ans->cmds[i]);
                                mqtt_publish(send);
                                data_free(send);
                        }
                }
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
        data_res *ans = data_decode_bdjs(rec);
        if (ans == NULL)
        {
                return ESP_OK;
        }
        else
        {
                for (uint8_t i = 0; i < 4; i++)
                {
                        gpio_input_num(cus_strip[i], ans->cmds[i]);
                }
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
        if (strncmp(call->recdata, "request", 7) == 0)
        {
                char *sysdata = data_get_sysmes();
                udp_client_sendto(call->addr, sysdata);
                free(sysdata);
        }
        else if (strncmp(call->recdata, "{", 1) == 0)
        {
                data_res *ans = data_decode_bdjs(call->recdata);
                //接收 其他物理开关 发送的指令
                if (ans->output0 != NULL)
                {
                        printf("udp get reversal %s \n", ans->output0);
                        //得到 cus_strip 序号
                        int index = atoi(ans->output0);
                        int res = gpio_input_reversal(cus_strip[index]);
                        char *send = data_bdjs_reported(KEYS[index], res);
                        mqtt_publish(send);
                        data_free(send);
                }
        }
        return ESP_OK;
}

//ota 检查结束
static esp_err_t ota_callback_handel()
{
        http_server_start();
        sntp_start(sntp_connect_callback);
        udp_client_start(udpcallback);
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
        GpioIni(); //默认全关
        nav_load_custom_data();
        wifi_connect_start(wifi_callback);
}