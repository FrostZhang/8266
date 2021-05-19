#ifndef UDPCOMPENT
#define UDPCOMPENT

typedef esp_err_t (*udp_callback_t)(char* callback,uint len);

void udp_client_send(const char *data);
void udp_client_start(udp_callback_t callback);

#endif