#ifndef APPLICATION_H
#define APPLICATION_H

#include "sb.h"      //stringbuilder
#include <string.h>  //string 库
#include <stdlib.h>  //std 库
#include <time.h>  //std 库
#include "esp_log.h" //标准的esp 打印
#include "esp_err.h" //标准的esp err 回调

#define APP_STRIP_4 1   //插排 4 孔
#define APP_STRIP_3 0   //插排 3 孔
#define APP_IR_RELAY 0  //红外 

void http_url_encode(char url[]);
void http_url_decode(char url[]);
void system_restart();
void system_pilot_light(int is_on);
void print_free_heap_size();
int strSearch(char *str1, char *str2);

extern char* XINHAO;
extern char* OTA_LABLE;
extern struct tm timeinfo;  //当前时间
extern int wifi_connect;    //当前是否连了wifi

#endif