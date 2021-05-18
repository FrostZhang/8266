#ifndef START_H
#define START_H

#include "httpcompent.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "time.h"

extern int gpio_isopen;
void ReStart();
esp_err_t httpcallback(http_event *call);
int get_isopen(gpio_num_t num);
void sntp_tick(struct tm* timeinfo);
void openFromDS(gpio_num_t num,int isopen);
void sys_light(int is_on);
#endif