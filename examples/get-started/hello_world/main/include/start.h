#ifndef START_H
#define START_H

#include "httpcompent.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "time.h"

esp_err_t system_http_callback(http_event *call);
int system_get_gpio_state(gpio_num_t num);
void system_sntp_callback(struct tm* timeinfo);
void system_ds_callback(gpio_num_t num,int isopen);
#endif