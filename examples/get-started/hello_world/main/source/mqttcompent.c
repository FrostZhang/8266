#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "mqttcompent.h"
#include "datacompent.h"
#include "netcompent.h"
#include "lwip/ip4_addr.h"

#include "nvs_flash.h" //载入资料
#include <string.h>

#define mqttport 1883
const char *mqtttag = "mqttcompent";
esp_mqtt_client_handle_t client;

const char *host = "t0eff28.mqtt.iot.gz.baidubce.com";
const char *mainid = "t0eff28/";
const char *upAcc = "$baidu/iot/shadow/%s/update/accepted";
const char *getAcc = "$baidu/iot/shadow/%s/get/accepted";
const char *get = "$baidu/iot/shadow/%s/get";
const char *up = "$baidu/iot/shadow/%s/update";

char *uptopic = {0};
char *userid = {0};
//             .username = "t0eff28/Asher8266",
//             .password = "uEzltFewLsAaMQnZ",
mqtt_callback_t callback;
int isConnect;

//连接成功后 注册百度相关的topic
void regist()
{
        // extern char *mqttusername;
        // char userid[32] = {0};
        // if (strncmp(mqttusername, mainid, strlen(mainid)) == 0)
        // {
        //         int n = 0;
        //         for (size_t i = strlen(mainid); i < strlen(mqttusername); i++)
        //         {
        //                 userid[n] = mqttusername[i];
        //                 n++;
        //         }
        //         userid[strlen(mqttusername) - strlen(mainid)] = '\0';
        // }
        // else
        // {
        //         strcpy(userid, mqttusername);
        //         userid[strlen(mqttusername)] = '\0';
        // }
        //printf("mqtt userid %s", userid);
        char sub[56] = {0};
        memset(sub, '\0', 56);
        sprintf(sub, upAcc, userid);
        esp_mqtt_client_subscribe(client, sub, 0);
        memset(sub, '\0', 56);
        sprintf(sub, getAcc, userid);
        esp_mqtt_client_subscribe(client, sub, 0);
        vTaskDelay(50 / portTICK_RATE_MS);
        char *send = setrequest(userid);
        memset(sub, '\0', 56);
        sprintf(sub, get, userid);
        //esp_mqtt_client_subscribe(client, sub, 0);
        esp_mqtt_client_publish(client, sub, send, 0, 0, 0);
        datafree(send);

        vTaskDelay(50 / portTICK_RATE_MS);
        extern ip4_addr_t *localIP;
        send = setreported2("local_ip", ip4addr_ntoa(localIP));
        ESP_LOGI(mqtttag, "send ip %s", send);
        // memset(sub, '\0', 56);
        // sprintf(sub, up, userid);
        // uptopic = os_malloc(strlen(sub) + 1);
        // strncpy(uptopic, sub, strlen(sub));
        // uptopic[strlen(sub)] = '\0';
        esp_mqtt_client_publish(client, uptopic, send, 0, 0, 0);
        datafree(send);
        datafree(userid);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
        esp_mqtt_client_handle_t client = event->client;

        //int msg_id;
        // your_context_t *context = event->context;
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
                ESP_LOGI(mqtttag, "MQTT_EVENT_CONNECTED");
                regist();
                isConnect = 1;
                break;
        case MQTT_EVENT_DISCONNECTED:
                isConnect = 0;
                ESP_LOGI(mqtttag, "MQTT_EVENT_DISCONNECTED");
                break;
        case MQTT_EVENT_SUBSCRIBED:

                break;
        case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(mqtttag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_PUBLISHED:
                //ESP_LOGI(mqtttag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_DATA:
                //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                event->data[event->data_len] = '\0';
                callback(event->data);
                break;
        case MQTT_EVENT_ERROR:
                ESP_LOGI(mqtttag, "MQTT_EVENT_ERROR");
                break;
        }
        return ESP_OK;
}

int mqtt_publish(const char *send)
{
        if (isConnect == 0 || uptopic == NULL)
        {
                return 0;
        }
        return esp_mqtt_client_publish(client, uptopic, send, 0, 0, 0);
}

void mqtt_stop()
{
        if (client != NULL)
        {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = NULL;
        }
}

//初始化userid 和 uptopic 作为遗嘱
void ini_mqtt_baidu(const char *mqttusername)
{
        userid = os_malloc(32);
        if (strncmp(mqttusername, mainid, strlen(mainid)) == 0)
        {
                int n = 0;
                for (size_t i = strlen(mainid); i < strlen(mqttusername); i++)
                {
                        userid[n] = mqttusername[i];
                        n++;
                }
                userid[strlen(mqttusername) - strlen(mainid)] = '\0';
        }
        else
        {
                strcpy(userid, mqttusername);
                userid[strlen(mqttusername)] = '\0';
        }
        ESP_LOGI(mqtttag, "ini uptopic");
        char sub[56] = {0};
        memset(sub, '\0', 56);
        sprintf(sub, up, userid);
        uptopic = os_malloc(strlen(sub) + 1);
        strncpy(uptopic, sub, strlen(sub));
        uptopic[strlen(sub)] = '\0';
}

esp_err_t mqtt_app_start(mqtt_callback_t call)
{
        mqtt_stop();

        extern char *mqttusername;
        extern char *mqttpassword;
        if (mqttusername == NULL || mqttpassword == NULL)
        {
                ESP_LOGE(mqtttag, "mqtt not config (baidu)");
                return ESP_FAIL;
        }
        callback = call;

        ESP_LOGI(mqtttag, "mqttusername %s %d", mqttusername, strlen(mqttusername));
        ESP_LOGI(mqtttag, "mqttpassword %s %d", mqttpassword, strlen(mqttpassword));
        ini_mqtt_baidu(mqttusername);

        //xEventGroupWaitBits(wifi_event_group, Net_SUCCESS,true, true, portMAX_DELAY);
        char *lwt_ms = setreported2("local_ip", "0.0.0.0");

        esp_mqtt_client_config_t mqtt_cfg = {
            .event_handle = mqtt_event_handler,
            .host = host,
            .port = mqttport,
            .username = mqttusername,
            .password = mqttpassword,
            .lwt_topic = uptopic,
            .lwt_msg = lwt_ms,
            .lwt_msg_len = strlen(lwt_ms),
            // .user_context = (void *)your_context
        };

        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_err_t err = esp_mqtt_client_start(client);
        if (err == ESP_FAIL)
        {
                esp_mqtt_client_destroy(client);
                client = NULL;
        }
        datafree(lwt_ms);
        return err;
}
