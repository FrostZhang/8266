
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
#include "driver/ledc.h"
#include "driver/ir_rx.h"
#include "driver/ir_tx.h"
#include "sh1106_s.h"
#include "udpcompent.h"
#include "sntpcompent.h"
#include "wificompent.h"
#include "datacompent.h"
#include "navcompent.h"
#include "mqttcompent.h"
#include "httpcompent.h"
#include "dscompent.h"
#include "iot_button.h"
#include "application.h"
#include "otacompent.h"

static const char *TAG = "Main";

#define LEDC_TEST_DUTY (4096)
#define LEDC_TEST_FADE_TIME (1500)

//常规：指示灯2(在芯片附近 烧录时会闪烁) 开关4/12/13/15 红外收5发14 中断0/3 TX1(不用) goio16(不用)
#define IR_RX_IO_NUM GPIO_NUM_5
#define IR_RX_BUF_LEN 128
#define IR_TX_IO_NUM GPIO_NUM_14

static int gpio_bit;

static void gpio_isr_handler(void *arg)
{
        uint32_t gpio_num = (uint32_t)arg;
        //xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
        gpio_isr_handler_remove(gpio_num); //防止接触不良多次触发

        vTaskDelay(700 / portTICK_RATE_MS);
        gpio_isr_handler_add(gpio_num, gpio_isr_handler, gpio_num);
}

//static xQueueHandle gpio_evt_queue = NULL;
//static void gpio_0_(void *arg)
// {
//         uint32_t io_num = (uint32_t)arg;
//         for (;;)
//         {
//                 if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
//                 {
//                         ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
//                 }
//         }
// }
// static void button_press_5s_cb(void *arg)
// {
//         print_free_heap_size();
// }

//初始化gpio
static void GpioIni(void)
{
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE; //取消中断
        io_conf.mode = GPIO_MODE_OUTPUT;       //对外输出 控制设备
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        io_conf.pin_bit_mask = GPIO_Pin_4 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
        gpio_config(&io_conf);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level(GPIO_NUM_4, 1);
        gpio_set_level(GPIO_NUM_12, 1);
        gpio_set_level(GPIO_NUM_13, 1);
        gpio_set_level(GPIO_NUM_15, 1);

        // button_handle_t btn_handle = iot_button_create(GPIO_NUM_0, BUTTON_ACTIVE_LOW);
        // iot_button_add_custom_cb(btn_handle, 5, button_press_5s_cb, NULL);
        //boot press
        //gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        //xTaskCreate(gpio_task_example, "gpio_task_example", 1024, NULL, 10, NULL);
#if defined(APP_STRIP_4) || defined(APP_STRIP_3)
        gpio_install_isr_service(0);
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT; //接收3.3v 输入
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        io_conf.pin_bit_mask = GPIO_Pin_0;
        gpio_config(&io_conf);
        gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);
        gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *)GPIO_NUM_0);

#endif
}

//ds18b20
static void Taskds18b20(void *p)
{
        uint8_t res = Ds18b20Init(GPIO_NUM_5);
        uint8_t temp = 0;
        if (res == 0)
        {
                ESP_LOGE(TAG, "18b20 ini failed");
                vTaskDelete(NULL);
        }
        while (1)
        {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                temp = (int)(Ds18b20ReadTemp() * 0.0625 + 0.005);
                printf("ds18b20采集的温度: %d \n\n", temp);
        }
}

//Dht11
static void TaskCreatDht11(void *p)
{
        uint8_t curTem = 0;
        uint8_t curHum = 0;
        uint8_t res = dh11Init();
        if (res == 0)
        {
                ESP_LOGE(TAG, "dh11Init failed");
                vTaskDelete(NULL);
        }
        while (1)
        {
                vTaskDelay(5000 / portTICK_RATE_MS);
                dh11Read(&curTem, &curHum);
                ESP_LOGI(TAG, "Temperature : %d , Humidity : %d", curTem, curHum);
        }
        vTaskDelete(NULL);
}

//呼吸灯
static void LEDC(void *p)
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

//红外接收
static void ir_rx_task(void *arg)
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
                ESP_LOGI(TAG, "addr1: 0x%x, addr2: 0x%x, cmd1: 0x%x, cmd2: 0x%x", ir_data.addr1, ir_data.addr2, ir_data.cmd1, ir_data.cmd2);

                if (ESP_OK == ir_rx_nec_code_check(ir_data))
                {
                        ESP_LOGI(TAG, "ir rx nec data:  0x%x", ir_data.cmd1);
                }
                else
                {
                        ESP_LOGI(TAG, "Non-standard nec infrared protocol");
                }
        }

        vTaskDelete(NULL);
}

//红外发射
static void ir_tx_task(void *arg)
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
                ESP_LOGI(TAG, "ir tx nec: addr:%02xh;cmd:%02xh;repeat:%d", ir_data[0].addr1, ir_data[0].cmd1, 4);
                ir_data[0].cmd1++;
                ir_data[0].cmd2 = ir_data[0].cmd1 + 1;
                vTaskDelay(1000 / portTICK_RATE_MS);
        }

        vTaskDelete(NULL);
}

//解析cmd 这是一个bit 值 包含1~32个开关 对应硬件的 gpio
static void gpio_input(int input)
{
        gpio_bit = input;
        if (input & GPIO_Pin_4)
                gpio_set_level(GPIO_NUM_4, 1);
        else
                gpio_set_level(GPIO_NUM_4, 0);
        if (input & GPIO_Pin_12)
                gpio_set_level(GPIO_NUM_12, 1);
        else
                gpio_set_level(GPIO_NUM_12, 0);
        if (input & GPIO_Pin_13)
                gpio_set_level(GPIO_NUM_13, 1);
        else
                gpio_set_level(GPIO_NUM_13, 0);
        if (input & GPIO_Pin_15)
                gpio_set_level(GPIO_NUM_15, 1);
        else
                gpio_set_level(GPIO_NUM_15, 0);
}

//定时器 回调
extern void system_ds_callback(gpio_num_t num, int isopen)
{
        if (isopen == 1)
                gpio_bit |= BIT(num);
        else
                gpio_bit &= ~BIT(num);

        printf("ds callback %d 回调 %d isopen %d\n", gpio_bit, num, isopen);
        gpio_input(gpio_bit);
        char *send = data_bdjs_reported(CMD, gpio_bit);
        mqtt_publish(send);
        data_free(send);
}

//判断gpio的开关状态
extern int system_get_gpio_state(gpio_num_t num)
{
        if (gpio_bit & BIT(num))
        {
                return 1;
        }
        return 0;
}

//http收到控制信息回调
extern esp_err_t system_http_callback(http_event *call)
{
        if (call->bdjs != NULL)
        {
                data_res *ans = data_decode_bdjs(call->bdjs);
                if (ans->cmd != -1)
                {
                        printf("HTTP get cmd %d \n", ans->cmd);
                        gpio_input(ans->cmd);
                        char *send = data_bdjs_reported(CMD, ans->cmd);
                        mqtt_publish(send);
                        data_free(send);
                }
        }
        else
        {
                int isopen = call->open4;
                int temp = gpio_bit;
                if (isopen != -1)
                {
                        if (isopen == 1)
                                temp |= GPIO_Pin_4;
                        else
                                temp &= ~GPIO_Pin_4;
                }
                isopen = call->open12;
                if (isopen != -1)
                {
                        if (isopen == 1)
                                temp |= GPIO_Pin_12;
                        else
                                temp &= ~GPIO_Pin_12;
                }
                isopen = call->open13;
                if (isopen != -1)
                {
                        if (isopen == 1)
                                temp |= GPIO_Pin_13;
                        else
                                temp &= ~GPIO_Pin_13;
                }
                isopen = call->open15;
                if (isopen != -1)
                {
                        if (isopen == 1)
                                temp |= GPIO_Pin_15;
                        else
                                temp &= ~GPIO_Pin_15;
                }
                if (temp != gpio_bit)
                {
                        gpio_bit = temp;
                        gpio_input(gpio_bit);
                        printf("HTTP get cmd %d \n", gpio_bit);
                        char *send = data_bdjs_reported(CMD, gpio_bit);
                        mqtt_publish(send);
                        data_free(send);
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
        if (ans->cmd != -1)
        {
                gpio_input(ans->cmd);
        }
        return ESP_OK;
}

static time_t now = 0;
//硬件时钟
static void hw_timer_callback(void *arg)
{
        time(&now);
        localtime_r(&now, &timeinfo); //更新时间
        if (timeinfo.tm_sec == 0)
        {
                extern uint32_t esp_get_time(void);
                ESP_LOGI(TAG, "esp_get_time %d", esp_get_time());

                ds_check(&timeinfo);
                print_free_heap_size();
        }
}
//定时器
static void hw_timer()
{
        hw_timer_init(hw_timer_callback, NULL);
        hw_timer_alarm_us(1000, 1);
        //hw_timer_disarm();
        //hw_timer_deinit();
}

//sntp 互联网可用?
static esp_err_t sntp_connect_callback(sntp_event *call)
{
        if (call->mestype == SNTP_EVENT_SUCCESS)
        {
                hw_timer();
                mqtt_app_start(mqttcallback);
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
extern esp_err_t udpcallback(char *rec, uint len)
{
        char *data = malloc(len);
        strncpy(data, rec, len);
        ESP_LOGI(TAG, "UDP REC %s", data);
        data_res *ans = data_decode_bdjs(data);
        if (ans->cmd != -1)
        {
                printf("udp get switchdata %d \n", ans->cmd);
                gpio_input(ans->cmd);
        }
        data_free(data);
        return ESP_OK;
}

//ota 检查结束
static esp_err_t ota_callback_handel()
{
        http_server_start();
        sntp_start(sntp_connect_callback);
        //udpclientstart(udpcallback);
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
        return ESP_OK;
}

//io14 io2
static esp_err_t oledini()
{
        esp_err_t err = oled_ini();
        if (err == ESP_OK)
        {
                oled_showStr(0, 2, "this is a test from asher 8266 !", 1);
        }
        return err;
}

static void adc_task()
{
        //int x;
        uint16_t adc_data[100];

        while (1)
        {
                if (ESP_OK == adc_read(&adc_data[0]))
                {
                        ESP_LOGI(TAG, "adc read: %d\r\n", adc_data[0]);
                }

                // ESP_LOGI(TAG, "adc read fast:\r\n");

                // if (ESP_OK == adc_read_fast(adc_data, 100)) {
                //     for (x = 0; x < 100; x++) {
                //         printf("%d\n", adc_data[x]);
                //     }
                // }

                vTaskDelay(1000 / portTICK_RATE_MS);
        }
}
//adc 检测
static void adc()
{
        adc_config_t adc_config;

        // Depend on menuconfig->Component config->PHY->vdd33_const value
        // When measuring system volTAGe(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
        adc_config.mode = ADC_READ_TOUT_MODE;
        adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
        ESP_ERROR_CHECK(adc_init(&adc_config));

        // 2. Create a adc task to read adc value
        xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
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
        ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());
}

void app_main()
{
        print_sys();
        GpioIni();

        nav_load_custom_data();
        wifi_connect_start(wifi_callback);

        //xTaskCreate(TaskCreatDht11, "TaskCreatDht11", 2048, NULL, 4, NULL);
        //xTaskCreate(Taskds18b20, "Taskds18b20", 2048, NULL, 3, NULL);
        //xTaskCreate(LEDC, "LEDC", 4096, NULL, 8, NULL);

        //io5
        xTaskCreate(ir_rx_task, "ir_rx_task", 2048, NULL, 5, NULL);
        //io14
        //xTaskCreate(ir_tx_task, "ir_tx_task", 2048, NULL, 5, NULL);
}