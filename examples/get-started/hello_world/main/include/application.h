#ifndef APPLICATION_H
#define APPLICATION_H

#include "sb.h"      //stringbuilder
#include <string.h>  //string 库
#include <stdlib.h>  //std 库
#include <time.h>  //std 库
#include "esp_log.h" //标准的esp 打印
#include "esp_err.h" //标准的esp err 回调

void http_url_encode(char url[]);
void http_url_decode(char url[]);
void system_restart();
void system_pilot_light(int is_on);
void print_free_heap_size();
int strSearch(char *str1, char *str2);
#endif