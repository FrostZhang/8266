
#include "application.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

#define BURSIZE 2048

char *XINHAO = "ledc";
char *OTA_LABLE;
struct tm timeinfo = {0};
int wifi_connect;
char *userid = {0}; //唯一ID 来自百度天工物

//16进制转10进制
extern int hex2dec(char c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    else if ('a' <= c && c <= 'f')
    {
        return c - 'a' + 10;
    }
    else if ('A' <= c && c <= 'F')
    {
        return c - 'A' + 10;
    }
    else
    {
        return -1;
    }
}

//10进制转16进制
extern char dec2hex(short int c)
{
    if (0 <= c && c <= 9)
    {
        return c + '0';
    }
    else if (10 <= c && c <= 15)
    {
        return c + 'A' - 10;
    }
    else
    {
        return -1;
    }
}

//编码一个url
extern void http_url_encode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i)
    {
        char c = url[i];
        if (('0' <= c && c <= '9') ||
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            c == '/' || c == '.')
        {
            res[res_len++] = c;
        }
        else
        {
            int j = (short int)c;
            if (j < 0)
                j += 256;
            int i1, i0;
            i1 = j / 16;
            i0 = j - i1 * 16;
            res[res_len++] = '%';
            res[res_len++] = dec2hex(i1);
            res[res_len++] = dec2hex(i0);
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

// 解码url
extern void http_url_decode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[BURSIZE];
    for (i = 0; i < len; ++i)
    {
        char c = url[i];
        if (c != '%')
        {
            res[res_len++] = c;
        }
        else
        {
            char c1 = url[++i];
            char c0 = url[++i];
            int num = 0;
            num = hex2dec(c1) * 16 + hex2dec(c0);
            res[res_len++] = num;
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

//设备重启
extern void system_restart()
{
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

//指示灯 gpio2
extern void system_pilot_light(int is_on)
{
    gpio_set_level(GPIO_NUM_2, is_on);
}

extern void print_free_heap_size()
{
    int size = esp_get_free_heap_size();
    printf("esp_system free heap size: %d\n", size);
    if (size<5000)
    {
        system_restart();
    }
    
}

int strSearch(char *str1, char *str2)
{
    int at, flag = 1;
    if (strlen(str2) > strlen(str1))
    {
        at = -1;
    }
    else if (!strcmp(str1, str2))
    {
        at = 0;
    }
    else
    {
        int i = 0, j = 0;
        for (i = 0; i < strlen(str1) && flag;)
        {
            for (j = 0; j < strlen(str2) && flag;)
            {
                if (str1[i] != str2[j])
                {
                    i++;
                    j = 0;
                }
                else if (str1[i] == str2[j])
                {
                    i++;
                    j++;
                }
                if (j == strlen(str2))
                {
                    flag = 0;
                }
            }
        }
        at = i - j;
    }
    return at;
}

char *substring(char *src, int pos, int length)
{
    //通过calloc来分配一个length长度的字符数组，返回的是字符指针。
    char *subch = (char *)calloc(sizeof(char), length + 1);
    //只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性。
    src = src + pos;
    //是pch指针指向pos位置。
    for (int i = 0; i < length; i++)
    {
        //        *(src++)将指针向前移动一位
        subch[i] = *(src++);
        //循环遍历赋值数组。
    }
    subch[length] = '\0'; //加上字符串结束符。
    return subch;         //返回分配的字符数组地址。
}
