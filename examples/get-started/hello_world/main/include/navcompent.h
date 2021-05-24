#ifndef NVACOMPENT_H
#define NVACOMPENT_H
#include "esp_err.h"

extern char dsdata[33];
extern char dsdata1[33];
extern char dsdata2[33];
extern char dsdata3[33];
extern char *wifissid;
extern char *wifipassword;
extern char *mqttusername;
extern char *mqttpassword;
extern char *ota_url;
extern int isr_gpio0_for;
extern int isr_gpio5_for;
extern int isr_gpio14_for;
extern int isr_gpio12_for;
extern char *wifi_sta_name;

void nav_load_custom_data();
esp_err_t nav_write_wifi(char ssid[32], char pass[64]);
esp_err_t nav_write_mqtt_baidu_account(char ssid[32], char pass[64]);
esp_err_t nav_write_ds(char dso[32], int num);
esp_err_t nav_write_ota(char otapath[128]);
esp_err_t nav_write_wifi_sta_name(char sta_name[32]);
#endif