#ifndef HTTPCOMPENT_H
#define HTTPCOMPENT_H
#include "esp_err.h"

typedef struct http_event
{
    int restart;
    int gpio;
    int gpio_level; //0 on 1 off 2 reval
    char *bdjs;
    int cmd; //
} http_event;

typedef http_event *http_event_handle_t;
typedef esp_err_t (*http_event_callback_t)(http_event_handle_t event);

esp_err_t http_server_start();
esp_err_t http_server_end();

#endif