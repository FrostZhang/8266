
#include <esp_http_server.h>
#include "stdlib.h"
#include "httpcompent.h"
#include <sys/param.h> //MIN

#include "navcompent.h"
#include "start.h"
#include <string.h>
#include "sntpcompent.h"
#include "application.h"

static const char *MQTTZZ = "mqttzz";
static const char *MQTTMM = "mqttmm";
static const char *OPENSTR = "open";
static const char *RESTART = "restart";
static const char *BDJS = "bdjs";
static const char *DATA = "data";
static const char *SPACE = " ";
static const char *LOGIN = "login";
static const char *ISR = "isr";
static const char *ISRP = "isrp";

static const char *TAG = "http";

static httpd_handle_t server = NULL;
static http_event httpevent;

static void reset_callback_data()
{
    httpevent.gpio = -1;
    httpevent.gpio_level = -1;
    httpevent.restart = -1;
    httpevent.bdjs = NULL;
}

static esp_err_t index_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;
    reset_callback_data();
    while (remaining > 0)
    {
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
        ESP_LOGI(TAG, "RECEIVED DATA %.*s", ret, buf);
        buf[ret] = '\0';
        http_url_decode(buf);

        char *mqttzz = NULL;
        char *mqttmm = NULL;
        int isrindex = -1; //0-3
        int isrvalue = -1;
        char *isrpath = NULL;
        char *ptr;
        char *p;
        ptr = strtok_r(buf, "&", &p);
        while (ptr != NULL)
        {
            char *p2;
            char *newp = ptr;
            char *key = strtok_r(newp, "=", &p2);
            char *value = strtok_r(NULL, "=", &p2);

            if (strcmp(key, MQTTZZ) == 0)
                mqttzz = value;
            else if (strcmp(key, MQTTMM) == 0)
                mqttmm = value;
            else if (strncmp(key, OPENSTR, 4) == 0)
            {
                char *io = substring(key, strlen(OPENSTR), strlen(key) - strlen(OPENSTR));
                httpevent.gpio = atoi(io);
                free(io);
                httpevent.gpio_level = atoi(value);
            }
            else if (strcmp(key, RESTART) == 0)
                httpevent.restart = 1;
            else if (strcmp(key, BDJS) == 0)
            {
                httpevent.bdjs = value;
                httpevent.bdjs[strlen(value)] = '\0';
            }
            else if (strncmp(key, ISRP, 4) == 0)
            {
                isrpath = value;
            }
            else if (strncmp(key, ISR, 3) == 0)
            {
                char *io = substring(key, strlen(ISR), strlen(key) - strlen(ISR));
                isrindex = atoi(io);
                free(io);
                isrvalue = atoi(value);
            }
            ptr = strtok_r(NULL, "&", &p);
        }
        if (mqttzz != NULL && mqttmm != NULL)
        {
            nav_write_mqtt_baidu_account(mqttzz, mqttmm);
            httpevent.restart = 1;
        }
        if (isrindex > -1)
        {
            nav_write_isr_for(isrindex, isrpath, isrvalue);
        }
    }

    system_http_callback(&httpevent);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
    httpd_resp_send_chunk(req, NULL, 0);
    printf("http Stack %ld\n", uxTaskGetStackHighWaterMark(NULL));
    return ESP_OK;
}

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static httpd_uri_t indexpost = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = index_post_handler,
    .user_ctx = index_html_start};

static esp_err_t indexget_handle(httpd_req_t *req)
{
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static httpd_uri_t indexget = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = indexget_handle,
    .user_ctx = index_html_start};

static esp_err_t ds_handle(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;

    buf_len = httpd_req_get_hdr_value_len(req, DATA) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, DATA, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => data: %s", buf);
            if (buf_len >= 32)
            {
                buf[buf_len] = '\0';
                int gpio = (buf[12] - '0') * 10 + (buf[13] - '0');
                nav_write_ds(buf, gpio);
            }
        }
        free(buf);
    }

    StringBuilder *sb = sb_create();
    sb_append(sb, dsdata);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata1);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata2);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata3);
    char *ds = sb_concat(sb);
    ESP_LOGI(TAG, "Send dsfatas => data: %s", ds);
    httpd_resp_send(req, ds, strlen(ds));
    free(ds);
    sb_free(sb);
    return ESP_OK;
}

static httpd_uri_t ds = {
    .uri = "/ds",
    .method = HTTP_GET,
    .handler = ds_handle,
    .user_ctx = NULL};

static esp_err_t heartbeat_handle(httpd_req_t *req)
{
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    return httpd_resp_send(req, strftime_buf, strlen(strftime_buf));
}

static httpd_uri_t heartbeat = {
    .uri = "/heartbeat",
    .method = HTTP_GET,
    .handler = heartbeat_handle,
    .user_ctx = NULL};

static esp_err_t htmlData_handle(httpd_req_t *req)
{
    StringBuilder *sb = sb_create();
    sb_appendf(sb, "cmd=%d,", gpio_bit);
    if (wifissid != NULL)
        sb_appendf(sb, "ssid=%s", wifissid);
    if (wifipassword != NULL)
        sb_appendf(sb, ",pass=%s", wifipassword);
    if (mqttusername != NULL)
        sb_appendf(sb, ",mqttzz=%s", mqttusername);
    if (mqttpassword != NULL)
        sb_appendf(sb, ",mqttmm=%s", mqttpassword);
    sb_appendf(sb, ",xinghao=%s", XINHAO);
    sb_appendf(sb, ",otachoose=%s", OTA_LABLE);
    if (ota_url != NULL)
    {
        sb_appendf(sb, ",ota_url=%s", ota_url);
    }
    if (isr_events[0].http != NULL)
        sb_appendf(sb, ",isrp0=%s", isr_events[0].http);
    else
        sb_appendf(sb, ",isr0=%d", isr_events[0].gpio);
    if (isr_events[0].http != NULL)
        sb_appendf(sb, ",isrp1=%s", isr_events[1].http);
    else
        sb_appendf(sb, ",isr1=%d", isr_events[1].gpio);
    if (isr_events[0].http != NULL)
        sb_appendf(sb, ",isrp2=%s", isr_events[2].http);
    else
        sb_appendf(sb, ",isr2=%d", isr_events[2].gpio);
    if (isr_events[0].http != NULL)
        sb_appendf(sb, ",isrp3=%s", isr_events[3].http);
    else
        sb_appendf(sb, ",isr3=%d", isr_events[3].gpio);

    sb_appendf(sb, ",sta_na=%d", wifi_sta_name);
    char *str = sb_concat(sb);
    esp_err_t err = httpd_resp_send(req, str, strlen(str));
    free(str);
    sb_free(sb);
    return err;
}

static httpd_uri_t htmlData = {
    .uri = "/htmlData",
    .method = HTTP_GET,
    .handler = htmlData_handle,
    .user_ctx = NULL};

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 1024 * 10;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &indexget);
        httpd_register_uri_handler(server, &indexpost);
        httpd_register_uri_handler(server, &htmlData);
        httpd_register_uri_handler(server, &ds);
        httpd_register_uri_handler(server, &heartbeat);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

//开启 http server
extern esp_err_t http_server_start()
{
    if (server != NULL)
        return;
    httpd_handle_t handle = start_webserver();
    if (handle != NULL)
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

//关闭 http server
extern esp_err_t http_server_end()
{
    if (server != NULL)
    {
        ESP_LOGI(TAG, "断开http");
        esp_err_t err = stop_webserver(server);
        server = NULL;
        return err;
    }
    return ESP_FAIL;
}