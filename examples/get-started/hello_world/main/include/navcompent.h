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

void nav_load_custom_data();
esp_err_t nav_write_wifi(char ssid[32], char pass[64]);
esp_err_t nav_write_mqtt_baidu_account(char ssid[32], char pass[64]);
esp_err_t nav_write_ds(char dso[32], int num);
#endif