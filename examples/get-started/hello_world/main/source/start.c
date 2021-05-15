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

#include "driver/i2c.h"
#include "driver/spi.h"

#include "lwip/apps/sntp.h"

#include "mqtt_client.h"
#include "os.h"
#include "dht.h"
#include "ds18b20.h"

#include "driver/ledc.h"
#include "driver/ir_rx.h"
#include "driver/ir_tx.h"
#include "sh1106_s.h"
#include "udpcompent.h"
#include "sntpcompent.h"
#include "netcompent.h"
#include "datacompent.h"
#include "navcompent.h"
#include "mqttcompent.h"
#include "httpcompent.h"
#include "dscompent.h"
#include "iot_button.h"

const char *tag = "main";

#define LEDC_TEST_DUTY (4096)
#define LEDC_TEST_FADE_TIME (1500)

//灯 2 开关 4 12 13 15 红外收 5 红外发 14
#define IR_RX_IO_NUM 5
#define IR_RX_BUF_LEN 128
#define IR_TX_IO_NUM 14
int gpio_isopen;

static xQueueHandle gpio_evt_queue = NULL;

// static void gpio_isr_handler(void *arg)
// {
//         uint32_t gpio_num = (uint32_t)arg;
//         xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
// }

// static void gpio_task_example(void *arg)
// {
//         uint32_t io_num;
//         for (;;)
//         {
//                 if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
//                 {
//                         ESP_LOGI(tag, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
//                 }
//         }
// }

void button_press_5s_cb(void *arg)
{
        ESP_LOGI(tag, "press 5s, heap: %d\n", esp_get_free_heap_size());
}

void sys_light(int is_on)
{
        gpio_set_level(GPIO_NUM_2, is_on);
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
        io_conf.pin_bit_mask = GPIO_Pin_12;
        gpio_config(&io_conf);
        io_conf.pin_bit_mask = GPIO_Pin_13;
        gpio_config(&io_conf);
        io_conf.pin_bit_mask = GPIO_Pin_15;
        gpio_config(&io_conf);

        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_NUM_4, 1);
        gpio_set_level(GPIO_NUM_12, 1);
        gpio_set_level(GPIO_NUM_13, 1);
        gpio_set_level(GPIO_NUM_15, 1);

        //gpio_install_isr_service(0); //中断都要初始化  //BUTTON_ACTIVE_LOW
        button_handle_t btn_handle = iot_button_create(GPIO_NUM_0, BUTTON_ACTIVE_LOW);
        iot_button_add_custom_cb(btn_handle, 5, button_press_5s_cb, NULL);
        // //boot press
        // io_conf.intr_type = GPIO_INTR_DISABLE;
        // io_conf.mode = GPIO_MODE_INPUT;
        // io_conf.pin_bit_mask = GPIO_Pin_0;
        // io_conf.pull_down_en = 0;
        // io_conf.pull_up_en = 0;
        // gpio_config(&io_conf);

        // gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);
        // gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        // xTaskCreate(gpio_task_example, "gpio_task_example", 1024, NULL, 10, NULL);

        // gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *)GPIO_NUM_0);
}

void ReStart()
{
        //设备重启
        printf("Restarting now.\n");
        fflush(stdout);
        esp_restart();
}

void Taskds18b20(void *p)
{
        uint8_t res = Ds18b20Init();
        uint8_t temp = 0;
        if (res == 0)
        {
                ESP_LOGE(tag, "18b20 ini failed");
                //vTaskDelete(NULL);
        }
        while (1)
        {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                temp = (int)(Ds18b20ReadTemp() * 0.0625 + 0.005);
                printf("ds18b20采集的温度: %d \n\n", temp);
                cJSON *jsonSend = cJSON_CreateObject();
                //cJSON_AddItemToObject(jsonSend, "reported", cJSON_CreateNumber(localIP->addr));
                cJSON *reported = cJSON_CreateObject();
                cJSON_AddItemToObject(jsonSend, "reported", reported);
                cJSON_AddItemToObject(reported, "temp", cJSON_CreateNumber(temp));
                char *sendstr = cJSON_PrintUnformatted(jsonSend);
                udp_client_send(sendstr);
                if (NULL != sendstr)
                {
                        cJSON_free(sendstr);
                }
                cJSON_Delete(jsonSend);
        }
}

void TaskCreatDht11(void *p)
{
        uint8_t curTem = 0;
        uint8_t curHum = 0;
        uint8_t res = dh11Init();
        if (res == 0)
        {
                ESP_LOGE(tag, "dh11Init failed");
                //vTaskDelete(NULL);
        }
        while (1)
        {
                vTaskDelay(5000 / portTICK_RATE_MS);
                dh11Read(&curTem, &curHum);
                ESP_LOGI(tag, "Temperature : %d , Humidity : %d", curTem, curHum);
                cJSON *jsonSend = cJSON_CreateObject();
                //cJSON_AddItemToObject(jsonSend, "reported", cJSON_CreateNumber(localIP->addr));
                cJSON *reported = cJSON_CreateObject();
                cJSON_AddItemToObject(jsonSend, "reported", reported);
                cJSON_AddItemToObject(reported, "temp", cJSON_CreateNumber(curTem));
                cJSON_AddItemToObject(reported, "hum", cJSON_CreateNumber(curHum));
                char *sendstr = cJSON_PrintUnformatted(jsonSend);
                udp_client_send(sendstr);
                if (NULL != sendstr)
                {
                        cJSON_free(sendstr);
                }
                cJSON_Delete(jsonSend);
        }
        vTaskDelete(NULL);
}

void LEDC(void *p)
{
        ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 5000,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0             // timer index
        };
        // Set configuration of timer0 for high speed channels
        ledc_timer_config(&ledc_timer);
        ledc_channel_config_t ledc_channel = {
            .channel = LEDC_CHANNEL_0,
            .duty = 0,
            .gpio_num = 12,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_TIMER_0};

        ledc_channel_config(&ledc_channel);
        ledc_fade_func_install(0);
        while (1)
        {
                ledc_set_fade_with_time(ledc_channel.speed_mode,
                                        ledc_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
                ledc_fade_start(ledc_channel.speed_mode,
                                ledc_channel.channel, LEDC_FADE_NO_WAIT);
                vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
                ledc_set_fade_with_time(ledc_channel.speed_mode,
                                        ledc_channel.channel, 0, LEDC_TEST_FADE_TIME);
                ledc_fade_start(ledc_channel.speed_mode,
                                ledc_channel.channel, LEDC_FADE_NO_WAIT);
                vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        }
}

static esp_err_t ir_rx_nec_code_check(ir_rx_nec_data_t nec_code)
{

        if ((nec_code.addr1 != ((~nec_code.addr2) & 0xff)))
        {
                return ESP_FAIL;
        }

        if ((nec_code.cmd1 != ((~nec_code.cmd2) & 0xff)))
        {
                return ESP_FAIL;
        }

        return ESP_OK;
}

void ir_rx_task(void *arg)
{
        ir_rx_nec_data_t ir_data;
        ir_rx_config_t ir_rx_config = {
            .io_num = IR_RX_IO_NUM,
            .buf_len = IR_RX_BUF_LEN};
        ir_rx_init(&ir_rx_config);

        while (1)
        {
                ir_data.val = 0;
                ir_rx_recv_data(&ir_data, 1, portMAX_DELAY);
                ESP_LOGI(tag, "addr1: 0x%x, addr2: 0x%x, cmd1: 0x%x, cmd2: 0x%x", ir_data.addr1, ir_data.addr2, ir_data.cmd1, ir_data.cmd2);

                if (ESP_OK == ir_rx_nec_code_check(ir_data))
                {
                        ESP_LOGI(tag, "ir rx nec data:  0x%x", ir_data.cmd1);
                }
                else
                {
                        ESP_LOGI(tag, "Non-standard nec infrared protocol");
                }
        }

        vTaskDelete(NULL);
}

void ir_tx_task(void *arg)
{
        ir_tx_config_t ir_tx_config = {
            .io_num = IR_TX_IO_NUM,
            .freq = 38000,
            .timer = IR_TX_WDEV_TIMER // WDEV timer will be more accurate, but PWM will not work
        };

        ir_tx_init(&ir_tx_config);

        ir_tx_nec_data_t ir_data[5];
        /*
        The standard NEC ir code is:
        addr + ~addr + cmd + ~cmd
    */
        ir_data[0].addr1 = 0x55;
        ir_data[0].addr2 = ~0x55;
        ir_data[0].cmd1 = 0x00;
        ir_data[0].cmd2 = ~0x00;

        while (1)
        {
                for (int x = 1; x < 5; x++)
                { // repeat 4 times
                        ir_data[x] = ir_data[0];
                }
                ir_tx_send_data(ir_data, 5, portMAX_DELAY);
                ESP_LOGI(tag, "ir tx nec: addr:%02xh;cmd:%02xh;repeat:%d", ir_data[0].addr1, ir_data[0].cmd1, 4);
                ir_data[0].cmd1++;
                ir_data[0].cmd2 = ir_data[0].cmd1 + 1;
                vTaskDelay(1000 / portTICK_RATE_MS);
        }

        vTaskDelete(NULL);
}

void gpio_input(int gpiobits)
{
        gpio_isopen = gpiobits;
        if (gpiobits & GPIO_Pin_4)
                gpio_set_level(GPIO_NUM_4, 1);
        else
                gpio_set_level(GPIO_NUM_4, 0);
        if (gpiobits & GPIO_Pin_2)
                gpio_set_level(GPIO_NUM_2, 1);
        else
                gpio_set_level(GPIO_NUM_2, 0);
        // if (gpiobits & GPIO_Pin_5)
        //         gpio_set_level(GPIO_NUM_5, 1);
        // else
        //         gpio_set_level(GPIO_NUM_5, 0);
        if (gpiobits & GPIO_Pin_12)
                gpio_set_level(GPIO_NUM_12, 1);
        else
                gpio_set_level(GPIO_NUM_12, 0);

        if (gpiobits & GPIO_Pin_13)
                gpio_set_level(GPIO_NUM_13, 1);
        else
                gpio_set_level(GPIO_NUM_13, 0);

        if (gpiobits & GPIO_Pin_15)
                gpio_set_level(GPIO_NUM_15, 1);
        else
                gpio_set_level(GPIO_NUM_15, 0);
}

//被定时器 回调
void openFromDS(gpio_num_t num, int isopen)
{
        if (isopen == 1)
                gpio_isopen |= (1 << num);
        else
                gpio_isopen &= ~(1 << num);

        printf("openFromDS gpio_isopen %d 回调 %d isopen %d\n", gpio_isopen, num, isopen);
        gpio_input(gpio_isopen);
        char *send = setreported("cmd", gpio_isopen);
        mqtt_publish(send);
        datafree(send);
}

void sntp_tick(struct tm *timeinfo)
{
        ontick(timeinfo);
        if (timeinfo->tm_sec==0)
        {
                ESP_LOGI(tag, "Free heap size: %d\n", esp_get_free_heap_size());
        }
        return;
}

int get_isopen(gpio_num_t num)
{
        if (gpio_isopen & BIT(num))
        {
                return 1;
        }
        return 0;
}

esp_err_t httpcallback(http_event *call)
{
        if (call->bdjs != NULL)
        {
                ESP_LOGI(tag, "HTTP REC %s", call->bdjs);
                data_res *ans = getreported(call->bdjs);
                if (ans->cmd != -1)
                {
                        printf("HTTP get switchdata %d \n", ans->cmd);
                        gpio_input(ans->cmd);
                        char *send = setreported("cmd", ans->cmd);
                        mqtt_publish(send);
                        datafree(send);
                }
        }
        else
        {
                int isopen = call->open;
                if (isopen != -1)
                {
                        if (isopen == 1)
                                gpio_isopen |= GPIO_Pin_4;
                        else
                                gpio_isopen &= ~GPIO_Pin_4;
                        gpio_input(gpio_isopen);
                        char *send = setreported("cmd", gpio_isopen);
                        mqtt_publish(send);
                        datafree(send);
                }
        }
        if (call->restart == 1)
        {
                ReStart();
        }
        return ESP_OK;
}

esp_err_t mqttcallback(char *rec)
{
        data_res *ans = getreported(rec);
        if (ans->cmd != -1)
        {
                gpio_input(ans->cmd);
        }
        return ESP_OK;
}

esp_err_t sntpCallback(sntp_event *call)
{
        if (call->mestype == SNTP_EVENT_SUCCESS)
        {
                mqtt_app_start(mqttcallback);
        }
        return ESP_OK;
}

esp_err_t udpcallback(char *rec, uint len)
{
        char *data = os_malloc(len);
        strncpy(data, rec, len);
        ESP_LOGI(tag, "UDP REC %s", data);
        data_res *ans = getreported(data);
        if (ans->cmd != -1)
        {
                printf("udp get switchdata %d \n", ans->cmd);
                gpio_input(ans->cmd);
        }
        os_free(data);
        return ESP_OK;
}

esp_err_t netcall(net_callback call)
{
        if (call == NET_CONNNECT)
        {
                sntpstart(sntpCallback);
                datacompentini();
                http_start();
                //udpclientstart(udpcallback);
        }
        return ESP_OK;
}

//io14 io2
esp_err_t oledini()
{
        esp_err_t err = oled_ini();
        if (err == ESP_OK)
        {
                oled_showStr(0, 2, "this is a test from asher 8266 !", 1);
        }
        return err;
}

void print_sys()
{
        /* Print chip information */
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

        printf("silicon revision %d, ", chip_info.revision);

        printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
               (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
        ESP_LOGI(tag, "Free heap size: %d\n", esp_get_free_heap_size());
}

void app_main()
{
        print_sys();
        GpioIni();

        loadconfig();
        netstart(netcall);

        //xTaskCreate(TaskCreatDht11, "TaskCreatDht11", 2048, NULL, 4, NULL);
        //xTaskCreate(Taskds18b20, "Taskds18b20", 2048, NULL, 3, NULL);
        //xTaskCreate(LEDC, "LEDC", 4096, NULL, 8, NULL);

        //io5
        xTaskCreate(ir_rx_task, "ir_rx_task", 2048, NULL, 5, NULL);
        //io14
        //xTaskCreate(ir_tx_task, "ir_tx_task", 2048, NULL, 5, NULL);
}