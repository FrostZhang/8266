#ifndef UDPCOMPENT
#define UDPCOMPENT
#include "application.h"
#include "lwip/sockets.h"

typedef struct udp_event
{
    char *recdata;
    uint len;
    in_addr_t addr;
} udp_event;

typedef udp_event *udp_event_handle_t;
typedef esp_err_t (*udp_callback_t)(udp_event_handle_t handel);

void udp_client_bord(const char *data);
void udp_client_start(udp_callback_t callback);
void udp_client_sendto2(const char *ip, const char *data);
void udp_client_sendto(in_addr_t addr, const char *data);
void udp_client_stop();
#endif