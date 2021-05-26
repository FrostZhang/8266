#include "cJSON.h"

#include "datacompent.h"
#include "esp_log.h"
#include "stdio.h"

const char *REPORTED = "reported";   //reported
const char *CMD = "cmd";             //cmd
const char *CMD1 = "cmd1";           //cmd1
const char *OUTPUT0 = "output0";     //output0
const char *OUTPUT1 = "output1";     //output1
const char *LOCAL_IP = "local_ip";   //local_ip
const char *REQUESTID = "requestId"; //requestId

static const char *TAG = "data";

static cJSON *jsonSender;
static data_res callback_data;

//malloc data_res. creat josn
extern void data_initialize()
{
    jsonSender = cJSON_CreateObject();
    ESP_LOGD(TAG, "data_initialize");
}

//得到json {"reported":{key:number}} 用完后请调用 datafree
extern char *data_bdjs_reported(const char *key, int number)
{
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, REPORTED, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateNumber(number));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REPORTED);
    return res;
}

//得到json {"reported":{key:value}} 用完后请调用 datafree
extern char *data_bdjs_reported_string(const char *key, const char *value)
{
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, REPORTED, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateString(value));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REPORTED);
    return res;
}

//得到json {"requestId":id} 用完后请调用 datafree
extern char *data_bdjs_request(const char *id)
{
    cJSON_AddItemToObject(jsonSender, REQUESTID, cJSON_CreateString(id));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REQUESTID);
    return res;
}

//释放json生成的char*
extern void data_free(void *object)
{
    cJSON_free(object);
}

//将类似{"reported":{"key":value,"key2":value}} 解析成 data_res
extern data_res *data_decode_bdjs(char *data)
{
    cJSON *json = cJSON_Parse(data);
    callback_data.cmd = -1;
    if (NULL != json)
    {
        cJSON *reporter = cJSON_GetObjectItem(json, REPORTED);
        if (NULL != reporter)
        {
            cJSON *jsoncmd = cJSON_GetObjectItem(reporter, CMD);
            if (jsoncmd != NULL && cJSON_IsNumber(jsoncmd))
            {
                callback_data.cmd = jsoncmd->valueint;
            }

            cJSON *jsono0 = cJSON_GetObjectItem(reporter, OUTPUT0);
            if (jsono0 != NULL && cJSON_IsString(jsono0))
            {
                callback_data.output0 = jsoncmd->valuestring;
            }
        }
        cJSON_Delete(json);
    }
    return &callback_data;
}