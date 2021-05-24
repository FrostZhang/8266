
#include "nvs_flash.h" //载入资料
#include "application.h"
#include "navcompent.h"
#include "start.h"

static const char *TAG = "flash";

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

char *ota_url = {0}; //ota 升级地址 通常是 http://xxx.xxx/xxx

char *wifi_sta_name = {0}; //当连接wifi 对方显示我的设备名称

#if defined(APP_STRIP_4) || defined(APP_STRIP_3)
//中断 引发gpio转换状态
int isr_gpio0_for;
int isr_gpio5_for;
int isr_gpio14_for;
int isr_gpio3_for;
#endif

static void read_wifi()
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
            wifissid = malloc(sizeof(ssid));
            strncpy(wifissid, ssid, sizeof(ssid));
            ESP_LOGI(TAG, "read wifi ssid = %s ", wifissid);
        }
        else
        {
            wifissid = CONFIG_ESP_WIFI_SSID;
            ESP_LOGI(TAG, "read sysconfig wifi ssid = %s ", wifissid);
        }

        char pass[64] = {0};
        len = sizeof(pass);
        err = nvs_get_str(mHandleNvsRead, "pass", pass, &len);
        if (err == ESP_OK)
        {
            wifipassword = malloc(sizeof(pass));
            strncpy(wifipassword, pass, sizeof(pass));
            ESP_LOGI(TAG, "read wifi pass = %s", wifipassword);
        }
        else
        {
            wifipassword = CONFIG_ESP_WIFI_PASSWORD;
            ESP_LOGI(TAG, "read sysconfig wifi pass = %s ", wifipassword);
        }
    }
    else
    {
        wifissid = CONFIG_ESP_WIFI_SSID;
        wifipassword = CONFIG_ESP_WIFI_PASSWORD;
        ESP_LOGI(TAG, "read sysconfig wifi: %s %s", wifissid, wifipassword);
    }
    nvs_close(mHandleNvsRead);
}

static void read_mqtt_baidu()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("mqtt", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[64] = {0};
        uint32_t len = 0;
        err = nvs_get_str(mHandleNvsRead, "username", strtemp, &len);
        if (err == ESP_OK)
        {
            mqttusername = malloc(len + 1);
            strncpy(mqttusername, strtemp, len);
            ESP_LOGI(TAG, "get str mqttzz = %s len:%d", mqttusername, strlen(mqttusername));
        }
        err = nvs_get_str(mHandleNvsRead, "password", strtemp, &len);
        if (err == ESP_OK)
        {
            mqttpassword = malloc(len + 1);
            strncpy(mqttpassword, strtemp, len);
            ESP_LOGI(TAG, "get str mqttmm = %s len:%d", mqttpassword, strlen(mqttpassword));
        }
    }
    nvs_close(mHandleNvsRead);
}

static void read_ds()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("dstime", NVS_READWRITE, &mHandleNvsRead);
    //打开数据库，打开一个数据库就相当于会返回一个句柄
    if (err == ESP_OK)
    {
        //读取 字符串
        char strtemp[64] = {0};
        uint32_t len = sizeof(strtemp);
        err = nvs_get_str(mHandleNvsRead, "dsdata", strtemp, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata, strtemp, len);
            //dsdata[len + 1] = '\0';
            ESP_LOGI(TAG, "get str dsdata = %s ", dsdata);
        }
        else
        {
            ESP_LOGE(TAG, "get dsdata err %d", err);
        }
        err = nvs_get_str(mHandleNvsRead, "dsdata1", strtemp, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata1, strtemp, len);
            //dsdata1[sizeof(strtemp)] = '\0';
            ESP_LOGI(TAG, "get str dsdata1 = %s ", dsdata1);
        }
        else
        {
            ESP_LOGE(TAG, "get dsdata1 err %d", err);
        }
        err = nvs_get_str(mHandleNvsRead, "dsdata2", strtemp, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata2, strtemp, len);
            //dsdata2[sizeof(strtemp)] = '\0';
            ESP_LOGI(TAG, "get str dsdata2 = %s ", dsdata2);
        }
        else
        {
            ESP_LOGE(TAG, "get dsdata2 err %d", err);
        }
        err = nvs_get_str(mHandleNvsRead, "dsdata3", strtemp, &len);
        if (err == ESP_OK)
        {
            strncpy(dsdata3, strtemp, len);
            //dsdata3[sizeof(strtemp)] = '\0';
            ESP_LOGI(TAG, "get str dsdata3 = %s ", dsdata3);
        }
        else
        {
            ESP_LOGE(TAG, "get dsdata3 err %d", err);
        }
    }
    else
    {
        ESP_LOGE(TAG, "nvs_open err %d", err);
    }

    nvs_close(mHandleNvsRead);
}

static void read_app_config()
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("appconfig", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[128] = {0};
        memset(strtemp, '\0', 128);
        uint32_t len = 0;
        err = nvs_get_str(mHandleNvsRead, "ota_url", strtemp, &len);
        if (err == ESP_OK)
        {
            ota_url = malloc(len + 1);
            strncpy(ota_url, strtemp, len);
            ESP_LOGI(TAG, "get str ota_url = %s", ota_url);
        }
#if defined(APP_STRIP_4) || defined(APP_STRIP_3)
        err = nvs_get_i32(mHandleNvsRead, "isr_gpio0_for", &isr_gpio0_for);
        if (err != ESP_OK)
        {
            isr_gpio0_for = 4;
        }
        err = nvs_get_i32(mHandleNvsRead, "isr_gpio5_for", &isr_gpio0_for);
        if (err != ESP_OK)
        {
            isr_gpio5_for = 12;
        }
        err = nvs_get_i32(mHandleNvsRead, "isr_gpio14_for", &isr_gpio0_for);
        if (err != ESP_OK)
        {
            isr_gpio14_for = 13;
        }
        err = nvs_get_i32(mHandleNvsRead, "isr_gpio3_for", &isr_gpio0_for);
        if (err != ESP_OK)
        {
            isr_gpio3_for = 15;
        }
#endif
        memset(strtemp, '\0', 128);
        err = nvs_get_str(mHandleNvsRead, "wifi_sta_name", strtemp, &len);
        if (err == ESP_OK)
        {
            wifi_sta_name = malloc(len + 1);
            strncpy(wifi_sta_name, strtemp, len);
            ESP_LOGI(TAG, "get str wifi_sta_name = %s", wifi_sta_name);
        }
        else
        {
            wifi_sta_name = malloc(12);
            wifi_sta_name = "Asher link";
        }
    }
    else
    {
        isr_gpio0_for = 4;
        isr_gpio5_for = 12;
        isr_gpio14_for = 13;
        isr_gpio3_for = 15;
        wifi_sta_name = malloc(12);
        wifi_sta_name = "Asher link";
    }
    nvs_close(mHandleNvsRead);
}

//写入 WiFi 账号 密码
extern esp_err_t nav_write_wifi(char issid[32], char ipass[64])
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

//写入 对接 百度mqtt 的账号 密码
extern esp_err_t nav_write_mqtt_baidu_account(char ssid[32], char pass[64])
{
    nvs_handle mHandleNvsRead;

    esp_err_t err = nvs_open("mqtt", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[32] = {0};
        strcpy(strtemp, ssid);
        err = nvs_set_str(mHandleNvsRead, "strtemp", strtemp);

        char strtemp[64] = {0};
        strcpy(strtemp, pass);
        err = nvs_set_str(mHandleNvsRead, "strtemp", strtemp);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

//写入gpio定时
extern esp_err_t nav_write_ds(char dso[32], int num)
{
    nvs_handle mHandleNvsRead;
    esp_err_t err = nvs_open("dstime", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[33] = {0};
        strncpy(strtemp, dso, 32);
        strtemp[32] = '\0';
        if (num == 4)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata", strtemp);
            strncpy(dsdata, strtemp, 32);
            dsdata[32] = '\0';
            ESP_LOGI(TAG, "write_ds dsdata = %s", strtemp);
        }
        else if (num == 12)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata1", strtemp);
            strncpy(dsdata1, strtemp, 32);
            dsdata1[32] = '\0';
            ESP_LOGI(TAG, "write_ds dsdata1 = %s", strtemp);
        }
        else if (num == 13)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata2", strtemp);
            strncpy(dsdata2, strtemp, 32);
            dsdata2[32] = '\0';
            ESP_LOGI(TAG, "write_ds dsdata2 = %s", strtemp);
        }
        else if (num == 15)
        {
            err = nvs_set_str(mHandleNvsRead, "dsdata3", strtemp);
            strncpy(dsdata3, strtemp, 32);
            dsdata3[32] = '\0';
            ESP_LOGI(TAG, "write_ds dsdata3 = %s", strtemp);
        }
        else
        {
            ESP_LOGI(TAG, "write_ds dsdata err num = %d", num);
        }
    }
    nvs_commit(mHandleNvsRead);
    nvs_close(mHandleNvsRead);
    return err;
}

//写入 ota 升级地址 不要超过 127个字
extern esp_err_t nav_write_ota(char otapath[128])
{
    nvs_handle mHandleNvsRead;
    //将airkiss获取的wifi写入内存
    esp_err_t err = nvs_open("appconfig", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[128] = {0};
        strcpy(strtemp, otapath);
        strtemp[strnlen(otapath, 127)] = '\0';
        err = nvs_set_str(mHandleNvsRead, "ota_url", strtemp);

        if (ota_url != NULL)
            free(ota_url);
        ota_url = malloc(128);
        strcpy(ota_url, strtemp);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

//写入 wifi station name
extern esp_err_t nav_write_wifi_sta_name(char sta_name[32])
{
    nvs_handle mHandleNvsRead;
    //将airkiss获取的wifi写入内存
    esp_err_t err = nvs_open("appconfig", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        char strtemp[32] = {0};
        strcpy(strtemp, 32);
        strtemp[strnlen(sta_name, 31)] = '\0';
        err = nvs_set_str(mHandleNvsRead, "wifi_sta_name", strtemp);
        if (wifi_sta_name != NULL)
            free(wifi_sta_name);
        wifi_sta_name = malloc(strnlen(sta_name, 32) + 1);
        strncpy(wifi_sta_name, strtemp, 31);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

//写入中断 0 5 14 3 引发什么gpio转换状态 （模拟开关 控制灯泡）
extern esp_err_t nav_write_isr_for(int a, int b, int c, int d)
{
    nvs_handle mHandleNvsRead;
    //将airkiss获取的wifi写入内存
    esp_err_t err = nvs_open("appconfig", NVS_READWRITE, &mHandleNvsRead);
    if (err == ESP_OK)
    {
        isr_gpio0_for = a;
        err = nvs_set_i32(mHandleNvsRead, "isr_gpio0_for", isr_gpio0_for);
        isr_gpio5_for = a;
        err = nvs_set_i32(mHandleNvsRead, "isr_gpio5_for", isr_gpio5_for);
        isr_gpio14_for = a;
        err = nvs_set_i32(mHandleNvsRead, "isr_gpio14_for", isr_gpio14_for);
        isr_gpio3_for = a;
        err = nvs_set_i32(mHandleNvsRead, "isr_gpio3_for", isr_gpio3_for);
    }
    nvs_close(mHandleNvsRead);
    return err;
}

extern void nav_load_custom_data()
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
    read_app_config();
}