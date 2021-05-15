#ifndef NVACOMPENT_H
#define NVACOMPENT_H
#include "esp_err.h"

void loadconfig();
esp_err_t write_wifi(char ssid[32], char pass[64]);
esp_err_t write_mqtt_baidu(char ssid[32], char pass[64]);
esp_err_t write_ds(char dso[32], int num);
#endif