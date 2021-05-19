#ifndef SNTPCOMPENT_H
#define SNTPCOMPENT_H
#include "lwip/apps/sntp.h"
#include "esp_err.h"

#include "time.h"
extern struct tm timeinfo;

typedef enum
{
    SNTP_EVENT_ERROR = 0,
    SNTP_EVENT_SUCCESS,
    SNTP_EVENT_CONNNECTFAILED,
    SNTP_EVENT_TIMING,
} sntp_id_t;

typedef struct sntp_event
{
    sntp_id_t mestype;
    struct tm *timeinfo;
} sntp_event;

typedef sntp_event *sntp_event_handle_t;
typedef esp_err_t (*sntp_event_callback_t)(sntp_event_handle_t event);

void sntp_start(sntp_event_callback_t callback);

#endif