#include "navcompent.h"
#include <stdlib.h>
#include <stdio.h>
#include "driver/gpio.h"

#include "dscompent.h"
#include "start.h"

typedef struct
{
    int is_on;
    gpio_num_t gpio;
}dsres;

dsres back = {0};

static dsres *readdata(char *ds_data, struct tm *timeinfo)
{
    int wday = timeinfo->tm_wday;
    back.gpio = 0;
    back.is_on = 0;
    //0 1234567 89 1011 1213 14
    //0 0123456 12 30   04   1
    //启用 周 时 分 gpio 循
    if (ds_data[0] == '1')
    {
        if (ds_data[14] == '1') //周循环
        {
            if (ds_data[1 + wday] == '1')
            {
                if ((ds_data[8] - '0') * 10 + (ds_data[9] - '0') == timeinfo->tm_hour)
                {
                    if ((ds_data[10] - '0') * 10 + (ds_data[11] - '0') == timeinfo->tm_min)
                    {
                        if (timeinfo->tm_sec == 0)
                        {
                            //定时开
                            int gpio = (ds_data[12] - '0') * 10 + (ds_data[13] - '0');
                            back.is_on = 2;
                            back.gpio = gpio;
                        }
                    }
                }
            }
        }
        else //非周循环
        {
            if ((ds_data[8] - '0') * 10 + (ds_data[9] - '0') == timeinfo->tm_hour)
            {
                if ((ds_data[10] - '0') * 10 + (ds_data[11] - '0') == timeinfo->tm_min)
                {
                    if (timeinfo->tm_sec == 0)
                    {
                        //定时开
                        int gpio = (ds_data[12] - '0') * 10 + (ds_data[13] - '0');
                        ds_data[0] = '0';
                        nav_write_ds(ds_data, 0);
                        back.is_on = 2;
                        back.gpio = gpio;
                        // return back;
                    }
                }
            }
        }
    }
    //16 17-23 2425 2627 2829 30
    if (ds_data[16] == '1')
    {
        if (ds_data[30] == '1') //周循环
        {
            if (ds_data[17 + wday] == '1')
            {
                if ((ds_data[24] - '0') * 10 + (ds_data[25] - '0') == timeinfo->tm_hour)
                {
                    if ((ds_data[26] - '0') * 10 + (ds_data[27] - '0') == timeinfo->tm_min)
                    {
                        if (timeinfo->tm_sec == 0)
                        {
                            //定时关
                            int gpio = (ds_data[28] - '0') * 10 + (ds_data[29] - '0');
                            back.is_on = 1;
                            back.gpio = gpio;
                            // return back;
                        }
                    }
                }
            }
        }
        else //非周循环
        {
            if ((ds_data[24] - '0') * 10 + (ds_data[25] - '0') == timeinfo->tm_hour)
            {
                if ((ds_data[26] - '0') * 10 + (ds_data[27] - '0') == timeinfo->tm_min)
                {
                    if (timeinfo->tm_sec == 0)
                    {
                        //定时关
                        int gpio = (ds_data[28] - '0') * 10 + (ds_data[29] - '0');
                        ds_data[16] = '0';
                        nav_write_ds(ds_data, 0);
                        back.is_on = 1;
                        back.gpio = gpio;
                        // return back;
                        //openFromDS(GPIO_Pin_4, 0);
                    }
                }
            }
        }
    }
    return &back;
}

//检测定时器 是否到点
extern void ds_check(struct tm *timeinfo)
{
    // extern char dsdata[33];
    // extern char dsdata1[33];
    // extern char dsdata2[33];
    // extern char dsdata3[33];
    int len = sizeof(dsdata);
    if (len > 30)
    {
        dsres *read = readdata(dsdata, timeinfo);
        if (read->is_on != 0)
        {
            system_ds_callback(read->gpio, read->is_on - 1);
        }
    }
    len = sizeof(dsdata1);
    if (len > 30)
    {
        dsres *read = readdata(dsdata1, timeinfo);
        if (read->is_on != 0)
        {
            system_ds_callback(read->gpio, read->is_on - 1);
        }
    }
    len = sizeof(dsdata2);
    if (len > 30)
    {
        dsres *read = readdata(dsdata2, timeinfo);
        if (read->is_on != 0)
        {
            system_ds_callback(read->gpio, read->is_on - 1);
        }
    }
    len = sizeof(dsdata3);
    if (len > 30)
    {
        dsres *read = readdata(dsdata3, timeinfo);
        if (read->is_on != 0)
        {
            system_ds_callback(read->gpio, read->is_on - 1);
        }
    }
}