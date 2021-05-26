#ifndef NVACOMPENT_H
#define NVACOMPENT_H
#include "esp_err.h"
#include "application.h"

extern char dsdata[33];
extern char dsdata1[33];
extern char dsdata2[33];
extern char dsdata3[33];
extern char *wifissid;
extern char *wifipassword;
extern char *mqttusername;
extern char *mqttpassword;
extern char *ota_url;

struct isr_evevt
{
    char* http;
    int gpio;
};

#if defined(APP_STRIP_4) || defined(APP_STRIP_3)
//中断 引发gpio事件
extern struct isr_evevt isr_events[4];
#endif

extern char *wifi_sta_name;

void nav_load_custom_data();
esp_err_t nav_write_wifi(char ssid[32], char pass[64]);
esp_err_t nav_write_mqtt_baidu_account(char ssid[32], char pass[64]);
esp_err_t nav_write_ds(char dso[32], int num);
esp_err_t nav_write_ota(char otapath[128]);
esp_err_t nav_write_wifi_sta_name(char sta_name[32]);
#endif