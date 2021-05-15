#ifndef SNTPCOMPENT_H
#define SNTPCOMPENT_H

#include "esp_err.h"

#include "time.h"

typedef enum
{
    SNTP_EVENT_ERROR = 0,
    SNTP_EVENT_SUCCESS,
    SNTP_EVENT_CONNNECTFAILED,
} sntp_id_t;

typedef struct sntp_event
{
    sntp_id_t mestype;
    struct tm timeinfo;
} sntp_event;

typedef sntp_event *sntp_event_handle_t;
typedef esp_err_t (*sntp_event_callback_t)(sntp_event_handle_t event);

void sntpstart(sntp_event_callback_t callback);

#endif