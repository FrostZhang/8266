
#include "datacompent.h"
#include "application.h"
#include "navcompent.h"

const char *REPORTED = "reported";   //reported
const char *OUTPUT0 = "output0";     //output0
const char *OUTPUT1 = "output1";     //output1
const char *LOCAL_IP = "local_ip";   //local_ip
const char *REQUESTID = "requestId"; //requestId
char *KEYS[] = {"cmd0", "cmd1", "cmd2", "cmd3", "cmd4"};

static const char *TAG = "data";

static cJSON *jsonSender;
static data_res callback_data;
static const char *SPACE = " ";
static uint64_t increment;
//malloc data_res. creat josn
extern void data_initialize()
{
    if (jsonSender == NULL)
    {
        jsonSender = cJSON_CreateObject();
        ESP_LOGD(TAG, "data_initialize");
    }
    else
    {
        ESP_LOGD(TAG, "data already initialized");
    }
}

static void reset()
{
    memset(callback_data.cmds, -2, 5);
    if (callback_data.output0 != NULL)
    {
        free(callback_data.output0);
        callback_data.output0 = NULL;
    }
}

//得到json {"reported":{key:number}} 用完后请调用 datafree
extern char *data_bdjs_reported(const char *key, int number)
{
    if (cJSON_HasObjectItem(jsonSender, REPORTED))
        cJSON_DeleteItemFromObject(jsonSender, REPORTED);
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, REPORTED, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateNumber(number));
    if (userid != NULL)
    {
        increment++;
        char id[16] = {0};
        strcat(id, userid);
        char num[8] = {0};
        strcat(id, itoa(increment, num, 10));
        cJSON_AddItemToObject(jsonSender, REQUESTID, cJSON_CreateString(id));
    }
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REQUESTID);
    cJSON_DeleteItemFromObject(jsonSender, REPORTED);
    return res;
}

//得到json {"reported":{key:value}} 用完后请调用 datafree
extern char *data_bdjs_reported_string(const char *key, const char *value)
{
    if (cJSON_HasObjectItem(jsonSender, REPORTED))
        cJSON_DeleteItemFromObject(jsonSender, REPORTED);

    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, REPORTED, reporter);
    cJSON_AddItemToObject(reporter, key, cJSON_CreateString(value));
    if (userid != NULL)
    {
        increment++;
        char id[16] = {0};
        strcat(id, userid);
        char num[8] = {0};
        strcat(id, itoa(increment, num, 10));
        cJSON_AddItemToObject(jsonSender, REQUESTID, cJSON_CreateString(id));
    }
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REQUESTID);
    cJSON_DeleteItemFromObject(jsonSender, REPORTED);
    return res;
}

//开始写json 百度的协议 需要end!
extern cJSON *begin_write_data_bdjs()
{
    cJSON *reporter = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonSender, REPORTED, reporter);
    return reporter;
}

//添加key 百度的协议 需要end!
extern void add_write_data_bdjs(cJSON *reporter, const char *key, int number)
{
    if (!cJSON_HasObjectItem(reporter, key))
    {
         cJSON_AddItemToObject(reporter, key, cJSON_CreateNumber(number));
    }
}

//添加key 百度的协议 需要end!
extern void add_write_data_bdjs_s(cJSON *reporter, const char *key, const char * value)
{
    if (!cJSON_HasObjectItem(reporter, key))
    {
         cJSON_AddItemToObject(reporter, key, cJSON_CreateString(value));
    }
}

//得到json {"reported":{key:value,key2:value2,...}} 用完后请调用 datafree
extern char *end_write_data_bdjs()
{
     if (userid != NULL)
    {
        increment++;
        char id[16] = {0};
        strcat(id, userid);
        char num[8] = {0};
        strcat(id, itoa(increment, num, 10));
        cJSON_AddItemToObject(jsonSender, REQUESTID, cJSON_CreateString(id));
    }
    char *res = cJSON_PrintUnformatted(jsonSender);
    cJSON_DeleteItemFromObject(jsonSender, REQUESTID);
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
    reset();
    if (NULL != json)
    {
        cJSON *jsonrequest = cJSON_GetObjectItem(json, REQUESTID);
        if (jsonrequest != NULL && cJSON_IsString(jsonrequest))
        {
            if (userid != NULL && strncmp(jsonrequest->valuestring, userid, strlen(userid)) == 0)
            {
                ESP_LOGI(TAG, "ignore self mes !"); //忽略自己发的信息
                cJSON_Delete(json);
                return NULL;
            }
        }
        cJSON *reporter = cJSON_GetObjectItem(json, REPORTED);
        if (NULL != reporter)
        {
            for (uint8_t i = 0; i < 5; i++)
            {
                cJSON *jsoncmd = cJSON_GetObjectItem(reporter, KEYS[i]);
                if (jsoncmd != NULL && cJSON_IsNumber(jsoncmd))
                {
                    callback_data.cmds[i] = jsoncmd->valueint;
                }
                else    
                {
                    callback_data.cmds[i] = -2;
                }
            }
            cJSON *jsono0 = cJSON_GetObjectItem(reporter, OUTPUT0);
            if (jsono0 != NULL && cJSON_IsString(jsono0))
            {
                callback_data.output0 = malloc(strlen(jsono0->valuestring) + 1);
                strcpy(callback_data.output0, jsono0->valuestring);
            }
        }
        cJSON_Delete(json);
    }
    return &callback_data;
}

//获得设备所有状态 需要free 结果
extern char *data_get_sysmes()
{
    StringBuilder *sb = sb_create();
    sb_appendf(sb, "xh=%s", XINHAO);
    if (wifissid != NULL)
        sb_appendf(sb, ",ssid=%s", wifissid);
    if (wifipassword != NULL)
        sb_appendf(sb, ",pass=%s", wifipassword);
    if (mqttusername != NULL)
        sb_appendf(sb, ",mqttzz=%s", mqttusername);
    if (mqttpassword != NULL)
        sb_appendf(sb, ",mqttmm=%s", mqttpassword);
    sb_appendf(sb, ",oc=%s", OTA_LABLE);
    if (ota_url != NULL)
        sb_appendf(sb, ",ou=%s", ota_url);
    sb_appendf(sb, ",na=%s", wifi_sta_name);
    char *str = sb_concat(sb);
    sb_free(sb);
    return str;
}

//获得设备所有定时 需要free 结果
extern char *data_get_dsdata()
{
    StringBuilder *sb = sb_create();
    sb_append(sb, dsdata);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata1);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata2);
    sb_append(sb, SPACE);
    sb_append(sb, dsdata3);
    char *ds = sb_concat(sb);
    sb_free(sb);
    return ds;
}