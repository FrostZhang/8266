#include "nvs_flash.h" //载入资料
#include "start.h"
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "navcompent.h"

const char *navtag = "nav";
char *wifissid = {0};
char *wifipassword = {0};
char *mqttusername = {0};
char *mqttpassword = {0};
//定时
//0 1234567 89 1011 1213 14   16 17-23   2425 2627 2829 30
//1 0123456 12 30   04   1    0  0123456 0 8  3 0  0 4  1 
//启用 周 时 分 gpio 循        关 周       时   分   gpio 循
char dsdata[33];
char dsdata1[33];
char dsdata2[33];
char dsdata3[33];

void read_wifi()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &mHandleNvsRead);
    //打开数据库，打开一个数据库就相当于会返回一个句柄
    if (err == ESP_OK)
    {
        //读取 字符串
        char ssid[32] = {0};
        uint32_t len = sizeof(ssid);
        err = nvs_get_str(mHandleNvsRead, "ssid", ssid, &len);

        if (err == ESP_OK)
        {
            //wifissid = ssid;
            wifissid = malloc(sizeof(ssid));
            strncpy(wifissid, ssid, sizeof(ssid));
            ESP_LOGI(navtag, "get str ssid = %s ", wifissid);
        }

        char pass[64] = {0};
        len = sizeof(pass);
        err = nvs_get_str(mHandleNvsRead, "pass", pass, &len);
        if (err == ESP_OK)
        {
            wifipassword = malloc(sizeof(pass));
            strncpy(wifipassword, pass, sizeof(pass));
            ESP_LOGI(navtag, "get str pass = %s", wifipassword);
        }
    }
    nvs_close(mHandleNvsRead);
}

void read_mqtt_baidu()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("mqtt", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char username[32] = {0};
        uint32_t len = sizeof(username);
        err = nvs_get_str(mHandleNvsRead, "username", username, &len);
        if (err == ESP_OK)
        {
            mqttusername = malloc(sizeof(username));
            strncpy(mqttusername, username, sizeof(username));
            ESP_LOGI(navtag, "get str mqttzz = %s", mqttusername);
        }

        char password[64] = {0};
        len = sizeof(password);
        err = nvs_get_str(mHandleNvsRead, "password", password, &len);
        if (err == ESP_OK)
        {
            mqttpassword = malloc(sizeof(password));
            strncpy(mqttpassword, password, sizeof(password));
            ESP_LOGI(navtag, "get str mqttmm = %s", mqttpassword);
        }
    }
    nvs_close(mHandleNvsRead);
}

void read_ds()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("dstime", NVS_READWRITE, &mHandleNvsRead);
    //打开数据库，打开一个数据库就相当于会返回一个句柄
    if (err == ESP_OK)
    {
        //读取 字符串
        char open[64] = {0};
        uint32_t len = sizeof(open);
        err = nvs_get_str(mHandleNvsRead, "dsdata", open, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata, open, sizeof(open));
            dsdata[sizeof(open)] = '\0';
            ESP_LOGI(navtag, "get str dsdata = %s ", dsdata);
        }
        else
        {
            ESP_LOGE(navtag, "dsdata err %d", err);
        }
        err = nvs_get_str(mHandleNvsRead, "dsdata1", open, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata1, open, sizeof(open));
            dsdata1[sizeof(open)] = '\0';
            ESP_LOGI(navtag, "get str dsdata1 = %s ", dsdata1);
        }
        else
        {
            ESP_LOGE(navtag, "dsdata1 err %d", err);
        }
        err = nvs_get_str(mHandleNvsRead, "dsdata2", open, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata2, open, sizeof(open));
            dsdata2[sizeof(open)] = '\0';
            ESP_LOGI(navtag, "get str dsdata2 = %s ", dsdata2);
        }
        else
        {
            ESP_LOGE(navtag, "dsdata2 err %d", err);
        }

        err = nvs_get_str(mHandleNvsRead, "dsdata3", open, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata3, open, sizeof(open));
            dsdata3[sizeof(open)] = '\0';
            ESP_LOGI(navtag, "get str dsdata3 = %s ", dsdata3);
        }
        else
        {
            ESP_LOGE(navtag, "dsdata3 err %d", err);
        }
    }
    else
    {
        ESP_LOGE(navtag, "nvs_open err %d", err);
    }

    nvs_close(mHandleNvsRead);
}

esp_err_t write_wifi(char issid[32], char ipass[64])
{
    nvs_handle mHandleNvsRead;
    //将airkiss获取的wifi写入内存
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char ssid[32] = {0};
        strcpy(ssid, issid);
        err = nvs_set_str(mHandleNvsRead, "ssid", ssid);

        char pass[64] = {0};
        strcpy(pass, ipass);
        err = nvs_set_str(mHandleNvsRead, "pass", pass);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

esp_err_t write_mqtt_baidu(char ssid[32], char pass[64])
{
    nvs_handle mHandleNvsRead;

    esp_err_t err = nvs_open("mqtt", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char username[32] = {0};
        strcpy(username, ssid);
        err = nvs_set_str(mHandleNvsRead, "username", username);

        char password[64] = {0};
        strcpy(password, pass);
        err = nvs_set_str(mHandleNvsRead, "password", password);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

esp_err_t write_ds(char dso[32], int num)
{
    nvs_handle mHandleNvsRead;
    //将airkiss获取的wifi写入内存
    esp_err_t err = nvs_open("dstime", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char open[33] = {0};
        strncpy(open, dso, 32);
        //恰好32位数字  所以最后补个0
        open[32] = '\0';
        if (num == 4)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata", open);
            strncpy(dsdata, open, 32);
            dsdata[32] = '\0';
            ESP_LOGI(navtag, "write_ds dsdata = %s", open);
        }
        else if (num == 12)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata1", open);
            strncpy(dsdata1, open, 32);
            dsdata1[32] = '\0';
            ESP_LOGI(navtag, "write_ds dsdata1 = %s", open);
        }
        else if (num == 13)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata2", open);
            strncpy(dsdata2, open, 32);
            dsdata2[32] = '\0';
            ESP_LOGI(navtag, "write_ds dsdata2 = %s", open);
        }
        else if (num == 15)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata3", open);
            strncpy(dsdata3, open, 32);
            dsdata3[32] = '\0';
            ESP_LOGI(navtag, "write_ds dsdata3 = %s", open);
        }
        else
        {
            ESP_LOGI(navtag, "write_ds dsdata err num = %d", num);
        }
    }
    nvs_commit(mHandleNvsRead);
    nvs_close(mHandleNvsRead);
    return err;
}

void loadconfig()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // 如果NVS分区被占用则对其进行擦除
        // 并再次初始化
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    read_wifi();
    read_mqtt_baidu();
    read_ds();
}
