#include "cJSON.h"

#include "datacompent.h"
#include "esp_log.h"
#include "stdio.h"

const char *reportedkey = "reported";
const char *cmd = "cmd";
const char *datatag = "datacompent";
const char *cmd1key = "cmd1";
const char *output0 = "output0";
const char *output1 = "output1";
const char *local_ip = "local_ip";

cJSON *jsonSender;
data_res callback_data;

//malloc data_res. creat josn
void datacompentini()
{
    jsonSender = cJSON_CreateObject();
}

//need free. baidu json {"reported":{key:value}}
char *setreported(const char *key, int number)
{
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, reportedkey, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateNumber(number));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, reportedkey);
    return res;
}

//need free. baidu json {"reported":{key:value}}
char *setreported2(const char *key, const char *value)
{
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, reportedkey, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateString(value));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, reportedkey);
    return res;
}

//need free. baidu json {"requestId":"xxxxx"}
char *setrequest(const char* id)
{
    cJSON_AddItemToObject(jsonSender, "requestId", cJSON_CreateString(id));
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender,"requestId");
    return res;
}

//free data
void datafree(void *object)
{
    cJSON_free(object);
}

//dont free. baidu json {"reported":{"key":value,"key2":value}}
data_res *getreported(char *data)
{
    cJSON *json = cJSON_Parse(data);
    callback_data.cmd = -1;
    if (NULL != json)
    {
        cJSON *reporter = cJSON_GetObjectItem(json, reportedkey);
        if (NULL != reporter)
        {
            cJSON *Switch = cJSON_GetObjectItem(reporter, cmd);
            if (cJSON_IsNumber(Switch))
            {
                callback_data.cmd = Switch->valueint;
                //
            }
        }
        cJSON_Delete(json);
    }
    return &callback_data;
}