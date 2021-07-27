#include "ledccompent.h"
#include "driver/ledc.h"

#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_LS_TIMER LEDC_TIMER_1
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL LEDC_CHANNEL_2
#define LEDC_HS_CH3_CHANNEL LEDC_CHANNEL_3;
#define IR_RX_BUF_LEN 128
ledc_channel_config_t ledc_channel[4];
//static int gpio_r;
//static int gpio_g;
//static int gpio_b;
static const char *TAG = "ledc";
static int light_style;
static bool isini;
static int fadetime = 1000;
static int rainbow[6][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255}};
static int lumen = 16;
static void LEDC(void *p)
{
#if !defined(APP_LEDC)
    ESP_LOGE(TAG, "当前不是APP_LEDC模式");
    vTaskDelete(NULL);
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

    ledc_channel[3].channel = LEDC_HS_CH3_CHANNEL;
    ledc_channel[3].duty = 0;
    ledc_channel[3].speed_mode = LEDC_HS_MODE;
    ledc_channel[3].hpoint = 0;
    ledc_channel[3].timer_sel = LEDC_HS_TIMER;

    int ch;
    for (ch = 0; ch < 4; ch++)
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
    ledc_fade_func_uninstall();
    vTaskDelete(NULL);
}

//设备由一个彩灯和一个白/黄灯组成 分开控制
extern void ledc_ini(int r, int b, int g, int style, int huang)
{
    if (isini)
    {
        ESP_LOGE(TAG, "already have ledc");
        return;
    }
    if (r == b || r == g || b == g)
    {
        ESP_LOGE(TAG, "输入gpio err");
        return;
    }
    light_style = style;
    isini = 1;
    ledc_channel[0].gpio_num = r;
    ledc_channel[1].gpio_num = b;
    ledc_channel[2].gpio_num = g;
    ledc_channel[3].gpio_num = huang;
    xTaskCreate(LEDC, "LEDC", 1024 * 4, NULL, 8, NULL);
}

extern void ledc_change_state(int style)
{
    light_style = style;
}

//控制七彩灯
extern void ledc_set_colorful(int color[3])
{
    if (!isini)
        return;
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

//单独控制黄灯
extern void ledc_set_huang(int strength)
{
    if (!isini)
        return;
    ledc_set_fade_with_time(ledc_channel[3].speed_mode,
                            ledc_channel[3].channel, strength * lumen, 100);
    ledc_fade_start(ledc_channel[3].speed_mode,
                    ledc_channel[3].channel, LEDC_FADE_WAIT_DONE);
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

//color: r|g<<8|b<<16|a<<24
static int color[4];
static int colorblack[4] = {0};
extern void ledc_color(int setc)
{
    color[0] = (setc & 0xff);       //r
    color[1] = (setc >> 8) & 0xff;  //g
    color[2] = (setc >> 16) & 0xff; //b  //当r=g=100 b是彩灯fade时长
    color[3] = (setc >> 24) & 0xff; //光照强度 0-255
    ledc_set_lumen(color[3]);
    if (color[0] == 255 && color[1] == 255 && color[2] == 255)
    {
        //白色 米黄
        ledc_set_colorful(colorblack);
        ledc_set_huang(color[3]);
        ESP_LOGI(TAG, "set ledc write %d", color[3]);
    }
    else
    {
        ledc_set_huang(0);
        if (color[0] == 100 && color[1] == 100)
        {
            ledc_change_state(2);
            ledc_set_fadtime(color[2] * 100);
            ESP_LOGI(TAG, "set ledc rainbow %d", color[2]);
        }
        else
        {
            ledc_set_colorful(color);
            ESP_LOGI(TAG, "set ledc color %d %d %d", color[0], color[1], color[2]);
        }
    }
}

//直接对接百度新协议 w (0-100)
extern void ledc_color2(int cmds[5])
{
    if (cmds[3] != -2)
    {
        ledc_set_lumen(cmds[3]);

        //仅传了亮度  没有别的参数
        if (cmds[0] == -2 && cmds[4] == -2)
        {
            if (color[0] == 255 && color[1] == 255 && color[2] == 255)
            {
                //缓存的是白光
                ledc_set_colorful(colorblack);
                ledc_set_huang(lumen / 0.0625f);
            }
            else if (color[0] == 0 && color[1] == 0 && color[2] == 0)
            {
                //缓存的是黑光
                ledc_set_colorful(colorblack);
                ledc_set_huang(color[3]);
            }
            else
            {
                //缓存的是彩光
                ledc_set_huang(0);
                ledc_set_colorful(color);
            }
        }
    }

    //彩虹特效
    if (cmds[4] != -2 && cmds[4] > 0)
    {
        ledc_change_state(2);
        ledc_set_fadtime(cmds[4] * 100);
    }
    else if (cmds[0] == 255 && cmds[1] == 255 && cmds[2] == 255)
    {
        //解析白光
        ledc_set_colorful(colorblack);
        ledc_set_huang(lumen / 0.0625f);
        color[0] = cmds[0];
        color[1] = cmds[1];
        color[2] = cmds[2];
        ESP_LOGI(TAG, "set ledc write %f", lumen / 0.0625f);
    }
    else if (cmds[0] > -1)
    {
        //解析彩色 和黑色
        ledc_set_huang(0);
        ledc_set_colorful(cmds);
        color[0] = cmds[0];
        color[1] = cmds[1];
        color[2] = cmds[2];
        ESP_LOGI(TAG, "set ledc color %d %d %d", cmds[0], cmds[1], cmds[2]);
    }
}