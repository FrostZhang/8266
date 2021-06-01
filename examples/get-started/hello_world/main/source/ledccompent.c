#include "ledccompent.h"
#include "driver/ledc.h"

#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_LS_TIMER LEDC_TIMER_1
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL LEDC_CHANNEL_2
#define IR_RX_BUF_LEN 128
ledc_channel_config_t ledc_channel[3];
static int gpio_r;
static int gpio_g;
static int gpio_b;
static const char *TAG = "ledc";
static int light_style;
static bool isini;
static int fadetime = 1000;
static int rainbow[6][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255}};
static int lumen = 16;
static void LEDC(void *p)
{
#if !defined(APP_LEDC)
    ESP_LOGE(TAG, "当前不是APP_LEDC模式 不可以使用 gpio12");
    return;
#endif
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER            // timer index
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    // Prepare and set configuration of timer1 for low speed channels
    ledc_timer.speed_mode = LEDC_LS_MODE;
    ledc_timer.timer_num = LEDC_LS_TIMER;
    ledc_timer_config(&ledc_timer);

    ledc_channel[0].channel = LEDC_HS_CH0_CHANNEL;
    ledc_channel[0].duty = 0;
    ledc_channel[0].speed_mode = LEDC_HS_MODE;
    ledc_channel[0].hpoint = 0;
    ledc_channel[0].timer_sel = LEDC_HS_TIMER;

    ledc_channel[1].channel = LEDC_HS_CH1_CHANNEL;
    ledc_channel[1].duty = 0;
    ledc_channel[1].speed_mode = LEDC_HS_MODE;
    ledc_channel[1].hpoint = 0;
    ledc_channel[1].timer_sel = LEDC_HS_TIMER;

    ledc_channel[2].channel = LEDC_HS_CH2_CHANNEL;
    ledc_channel[2].duty = 0;
    ledc_channel[2].speed_mode = LEDC_HS_MODE;
    ledc_channel[2].hpoint = 0;
    ledc_channel[2].timer_sel = LEDC_HS_TIMER;

    int ch;
    for (ch = 0; ch < 3; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
    }
    ledc_fade_func_install(0);
    int loop = -1;
    while (isini)
    {
        if (light_style <= 0)
        {
            vTaskDelay(fadetime / portTICK_PERIOD_MS);
            continue;
        }

        loop++;
        if (loop >= 6)
        {
            loop = -1;
            continue;
        }

        if (light_style >= 1)
        {
            for (ch = 0; ch < 3; ch++)
            {
                ledc_set_fade_with_time(ledc_channel[ch].speed_mode,
                                        ledc_channel[ch].channel, rainbow[loop][ch] * lumen, fadetime);
                ledc_fade_start(ledc_channel[ch].speed_mode,
                                ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
            }
        }
        vTaskDelay(fadetime / portTICK_PERIOD_MS);
        if (light_style == 1)
        {
            for (ch = 0; ch < 3; ch++)
            {
                ledc_set_fade_with_time(ledc_channel[ch].speed_mode,
                                        ledc_channel[ch].channel, 0, fadetime);
                ledc_fade_start(ledc_channel[ch].speed_mode,
                                ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
            }
            vTaskDelay(fadetime / portTICK_PERIOD_MS);
        }
    }
}

extern void ledc_ini(int r, int b, int g, int style)
{
    if (r == b || r == g || b == g)
    {
        ESP_LOGE(TAG, "输入gpio err");
        return;
    }
    light_style = style;
    isini = style;
    ledc_channel[0].gpio_num = r;
    ledc_channel[1].gpio_num = b;
    ledc_channel[2].gpio_num = g;
    xTaskCreate(LEDC, "LEDC", 4096, NULL, 8, NULL);
}

extern void ledc_change_state(int style)
{
    light_style = style;
}

extern void ledc_setcolor(int color[3])
{
    if (light_style != 0)
    {
        light_style = 0;
        //vTaskDelay(fadetime / portTICK_PERIOD_MS);
    }
    //ESP_LOGI(TAG, "set color %d %d %d", color[0], color[1], color[2]);
    for (int ch = 0; ch < 3; ch++)
    {
        ledc_set_fade_with_time(ledc_channel[ch].speed_mode,
                                ledc_channel[ch].channel, color[ch] * lumen, 100);
        ledc_fade_start(ledc_channel[ch].speed_mode,
                        ledc_channel[ch].channel, LEDC_FADE_WAIT_DONE);
        // ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, color[ch]);
        // ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
    }
    //vTaskDelay(500 / portTICK_PERIOD_MS);
}

//输入彩虹变色速度 100-4000
extern void ledc_set_fadtime(int time)
{
    if (time < 100)
    {
        time = 100;
    }
    fadetime = time;
}

//输入光强度 0-255
extern void ledc_set_lumen(int lu)
{
    lumen = lu * 0.0625f;
}

extern void ledc_deini()
{
    isini = 0;
}