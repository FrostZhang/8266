#ifndef WIFI_COMPENT_H
#define WIFI_COMPENT_H
#include "esp_wifi.h"

extern ip4_addr_t *LocalIP;

typedef enum
{
    WIFI_ERR = 0,
    WIFI_CONNNECT,
    WIFI_Disconnect,
} net_callback;

typedef esp_err_t (*net_event_callback_t)(net_callback callback);
void wifi_connect_start(net_event_callback_t callback);

#endif