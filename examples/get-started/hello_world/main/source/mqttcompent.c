
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_client.h"
#include "application.h"
#include "mqttcompent.h"
#include "datacompent.h"
#include "wificompent.h"
#include "lwip/ip4_addr.h"

//#include "nvs_flash.h" //载入资料

#define mqttport 1883
static const char *TAG = "mqttcompent";
static esp_mqtt_client_handle_t client = {0};

static const char *host = "t0eff28.mqtt.iot.gz.baidubce.com";
static const char *mainid = "t0eff28/";
static const char *upAcc = "$baidu/iot/shadow/%s/update/accepted";
static const char *getAcc = "$baidu/iot/shadow/%s/get/accepted";
static const char *get = "$baidu/iot/shadow/%s/get";
static const char *up = "$baidu/iot/shadow/%s/update";

static char *uptopic = {0};
static char *userid = {0};
//             .username = "t0eff28/Asher8266",
//             .password = "uEzltFewLsAaMQnZ",
static mqtt_callback_t callback;
static int isConnect;

//连接成功后 注册百度相关的topic
static void regist()
{
        char sub[56] = {0};
        memset(sub, '\0', 56);
        sprintf(sub, upAcc, userid);
        esp_mqtt_client_subscribe(client, sub, 0);
        memset(sub, '\0', 56);
        sprintf(sub, getAcc, userid);
        esp_mqtt_client_subscribe(client, sub, 0);
        vTaskDelay(50 / portTICK_RATE_MS);
        char *send = data_bdjs_request(userid);
        memset(sub, '\0', 56);
        sprintf(sub, get, userid);
        //esp_mqtt_client_subscribe(client, sub, 0);
        esp_mqtt_client_publish(client, sub, send, 0, 0, 0);
        data_free(send);

        vTaskDelay(50 / portTICK_RATE_MS);
        //extern ip4_addr_t *localIP;
        send = data_bdjs_reported_string(LOCAL_IP, ip4addr_ntoa(LocalIP));
        ESP_LOGI(TAG, "send ip %s", send);
        esp_mqtt_client_publish(client, uptopic, send, 0, 0, 0);
        data_free(send);
        data_free(userid);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
        //esp_mqtt_client_handle_t client = event->client;
        //int msg_id;
        // your_context_t *context = event->context;
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
                regist();
                isConnect = 1;
                break;
        case MQTT_EVENT_DISCONNECTED:
                isConnect = 0;
                ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
                break;
        case MQTT_EVENT_SUBSCRIBED:

                break;
        case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_PUBLISHED:
                //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                break;
        case MQTT_EVENT_DATA:
                //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                event->data[event->data_len] = '\0';
                callback(event->data);
                printf("mqtt Stack %ld", uxTaskGetStackHighWaterMark(NULL));
                break;
        case MQTT_EVENT_ERROR:
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
                break;
        }
        return ESP_OK;
}

//mqtt发送
extern int mqtt_publish(const char *send)
{
        if (isConnect == 0 || uptopic == NULL)
        {
                return 0;
        }
        return esp_mqtt_client_publish(client, uptopic, send, 0, 0, 0);
}

//停止mqtt
extern void mqtt_stop()
{
        if (client != NULL)
        {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = NULL;
        }
}

//初始化userid 和 uptopic 作为遗嘱 userid需要释放
static void ini_mqtt_baidu(const char *mqttusername)
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
        ESP_LOGI(TAG, "ini uptopic");
        char sub[56] = {0};
        memset(sub, '\0', 56);
        sprintf(sub, up, userid);
        uptopic = os_malloc(strlen(sub) + 1);
        strncpy(uptopic, sub, strlen(sub));
        uptopic[strlen(sub)] = '\0';
}

//开始mqtt
extern esp_err_t mqtt_app_start(mqtt_callback_t call)
{
        mqtt_stop();

        extern char *mqttusername;
        extern char *mqttpassword;
        if (mqttusername == NULL || mqttpassword == NULL)
        {
                ESP_LOGE(TAG, "mqtt not config (baidu)");
                return ESP_FAIL;
        }
        callback = call;
        
        ini_mqtt_baidu(mqttusername);

        //xEventGroupWaitBits(wifi_event_group, Net_SUCCESS,true, true, portMAX_DELAY);
        char *lwt_ms = data_bdjs_reported_string(LOCAL_IP, "0.0.0.0");

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
        data_free(lwt_ms);
        return err;
}
