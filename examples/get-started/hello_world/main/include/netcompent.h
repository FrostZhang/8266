#ifndef NETCOMPENT_H
#define NETCOMPENT_H

typedef enum
{
    NET_ERR = 0,
    NET_CONNNECT,
    NET_Disconnect,
} net_callback;

typedef esp_err_t (*net_event_callback_t)(net_callback callback);
void netstart(net_event_callback_t callback);

#endif