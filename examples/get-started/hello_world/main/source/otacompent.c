
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "navcompent.h"
#include "application.h"
#include "otacompent.h"

static const char *TAG = "ota";
static ota_callback callback;
static TaskHandle_t handle;
static int check_count;
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

static void ota_task(void *pvParameter)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "running partition: %s\n", running->label);
    OTA_LABLE = strdup(running->label);

    ESP_LOGI(TAG, "Start to Connect to Server....");
    char *path = ota_url;
    if (path == NULL)
    {
        callback();
        vTaskDelete(NULL);
        return;
    }
    char *url = malloc(strlen(path) + 18);
    memset(url, '\0', strlen(path) + 18);
    strncpy(url, path, strlen(path));

    const esp_partition_t *notruning = esp_ota_get_next_update_partition(running);
    strcat(url, notruning->label);
    ESP_LOGI(TAG, "begin download: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        //.cert_pem = (char *)server_cert_pem_start,
        .cert_pem = NULL,
        .event_handler = _http_event_handler,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK)
    {
        system_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Firmware Upgrades Failed");
        free(url);
        free(path);
        callback();
        vTaskDelete(NULL);
    }
}

extern void ota_check(ota_callback call)
{
    if (check_count == 0)
    {
        check_count++;
        callback = call;
        xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, &handle);
    }
    else
    {
        ESP_LOGI(TAG, "already check ota version!");
        call();
    }
}