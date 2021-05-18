#include <esp_log.h>
#include <esp_http_server.h>
#include "stdlib.h"
#include "httpcompent.h"
#include <sys/param.h> //MIN

#include "navcompent.h"
#include "start.h"
#include <string.h>
#include "sntpcompent.h"
#include "help.h"

static const char *MQTTZZ = "mqttzz";
static const char *MQTTMM = "mqttmm";
static const char *OPEN4 = "open4";
static const char *OPEN12 = "open12";
static const char *OPEN13 = "open13";
static const char *OPEN15 = "open15";
static const char *BDJS = "bdjs";
static const char *DATA = "data";
static const char *SPACE = " ";

static const char *TAG = "http";

httpd_handle_t server = NULL;
http_event httpevent;

void reset_callback_data()
{
    httpevent.open4 = -1;
    httpevent.open12 = -1;
    httpevent.open13 = -1;
    httpevent.open15 = -1;
    httpevent.restart = -1;
    httpevent.bdjs = NULL;
}

esp_err_t index_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;
    reset_callback_data();
    while (remaining > 0)
    {
        /* Read the data for the request */
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
        /* Send back the same data */
        //httpd_resp_send_chunk(req, buf, ret);

        remaining -= ret;

        /* Log data received */
        // ESP_LOGI(httptag, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "RECEIVED DATA %.*s", ret, buf);
        // ESP_LOGI(httptag, "====================================");
        buf[ret] = '\0';
        urldecode(buf);

        char *mqttzz = NULL;
        char *mqttmm = NULL;
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
            {
                mqttzz = value;
            }
            if (strcmp(key, MQTTMM) == 0)
            {
                mqttmm = value;
            }
            if (strcmp(key, OPEN4) == 0)
            {
                httpevent.open4 = atoi(value);
            }
            if (strcmp(key, OPEN12) == 0)
            {
                httpevent.open12 = atoi(value);
            }
            if (strcmp(key, OPEN13) == 0)
            {
                httpevent.open13 = atoi(value);
            }
            if (strcmp(key, OPEN15) == 0)
            {
                httpevent.open15 = atoi(value);
            }
            //对接百度json
            if (strcmp(key, BDJS) == 0)
            {
                httpevent.bdjs = value;
                httpevent.bdjs[strlen(value)] = '\0';
                httpd_resp_send_chunk(req, "bdjsok", 2);
            }
            ptr = strtok_r(NULL, "&", &p);
        }
        if (mqttzz != NULL && mqttmm != NULL)
        {
            write_mqtt_baidu(mqttzz, mqttmm);
            httpevent.restart = 1;
        }
    }
    //const char *resp_str = (const char *)req->user_ctx;
    // End response
    httpcallback(&httpevent);

    vTaskDelay(500 / portTICK_PERIOD_MS);
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
    httpd_resp_send_chunk(req, NULL, 0);

    //httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

httpd_uri_t indexpost = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = index_post_handler,
    .user_ctx = index_html_start};

esp_err_t indexget_handle(httpd_req_t *req)
{
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

httpd_uri_t indexget = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = indexget_handle,
    .user_ctx = index_html_start};

esp_err_t ds_handle(httpd_req_t *req)
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
                write_ds(buf, gpio);
            }
        }
        free(buf);
    }
    // extern char dsdata[33];
    // extern char dsdata1[33];
    // extern char dsdata2[33];
    // extern char dsdata3[33];

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

httpd_uri_t ds = {
    .uri = "/ds",
    .method = HTTP_GET,
    .handler = ds_handle,
    .user_ctx = NULL};

esp_err_t heartbeat_handle(httpd_req_t *req)
{
    //struct tm timeinfo;
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    return httpd_resp_send(req, strftime_buf, strlen(strftime_buf));
}

httpd_uri_t heartbeat = {
    .uri = "/heartbeat",
    .method = HTTP_GET,
    .handler = heartbeat_handle,
    .user_ctx = NULL};

esp_err_t htmlData_handle(httpd_req_t *req)
{
    char str[128];
    memset(str, '\0', 128);

    if (gpio_bit & BIT4)
    {
        strcat(str, "open4=1,");
    }
    if (gpio_bit & BIT12)
    {
        strcat(str, "open12=1,");
    }
    if (gpio_bit & BIT13)
    {
        strcat(str, "open13=1,");
    }
    if (gpio_bit & BIT15)
    {
        strcat(str, "open15=1,");
    }
    // extern char *mqttusername;
    // extern char *mqttpassword;
    strcat(str, "mqttzz=");
    strcat(str, mqttusername);
    strcat(str, ",mqttmm=");
    strcat(str, mqttpassword);

    return httpd_resp_send(req, str, strlen(str));
}

httpd_uri_t htmlData = {
    .uri = "/htmlData",
    .method = HTTP_GET,
    .handler = htmlData_handle,
    .user_ctx = NULL};

httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

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

esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

//开启 http server
esp_err_t http_start()
{
    httpd_handle_t handle = start_webserver();
    if (handle != NULL)
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

//关闭 http server
esp_err_t http_end()
{
    if (server != NULL)
    {
        return stop_webserver(server);
    }
    return ESP_FAIL;
}