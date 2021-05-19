#ifndef MQTTCOMPENT_H
#define MQTTCOMPENT_H

#include "esp_err.h"

typedef esp_err_t (*mqtt_callback_t)(char* callback);

esp_err_t mqtt_app_start(mqtt_callback_t call);
int mqtt_publish(const char* send);
void mqtt_stop();
#endif