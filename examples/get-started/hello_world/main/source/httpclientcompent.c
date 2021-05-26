#include "esp_http_client.h"
#include "httpclientcompent.h"

static const char *TAG = "httpclient";
static esp_http_client_handle_t client;
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // Write out data
            // printf("%.*s", evt->data_len, (char*)evt->data);
        }
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

static void http_task(void *pvParameters)
{
    esp_http_client_config_t config = {
        .url = "http://0.0.0.0/",
        .event_handler = _http_event_handler,
        .port =8266
    };
    client = esp_http_client_init(&config);
}

extern void http_client_get(const char *url)
{
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_url(client, url);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %d", err);
    }
}

extern void http_client_post(const char *url, const char *data)
{
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data, strlen(data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %d", err);
    }
}

extern void http_clent_start()
{
    if (client == NULL)
    {
        xTaskCreate(&http_task, "http_task", 1024 * 8, NULL, 1, NULL);
    }
}