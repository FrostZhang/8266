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

void loadconfig();
esp_err_t write_wifi(char ssid[32], char pass[64]);
esp_err_t write_mqtt_baidu(char ssid[32], char pass[64]);
esp_err_t write_ds(char dso[32], int num);
#endif