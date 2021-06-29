#ifndef APPLICATION_H
#define APPLICATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sb.h"      //stringbuilder
#include <string.h>  //string 库
#include <stdlib.h>  //std 库
// #include <stdint.h>  //std 库
#include <time.h>  //std 库
#include "esp_log.h" //标准的esp 打印
#include "esp_err.h" //标准的esp err 回调
#include "driver/gpio.h"

#define ON 0
#define OPEN 0
#define OFF 1
#define CLOSE 1 

int hex2dec(char c);
char dec2hex(short int c);
void http_url_encode(char url[]);
void http_url_decode(char url[]);
void system_restart();
void system_pilot_light(int is_on);
void print_free_heap_size();
int strSearch(char *str1, char *str2);
char *substring(char *src, int pos, int length);
extern int system_get_gpio_state(gpio_num_t num);

extern char* XINHAO;        //型号
extern char* OTA_LABLE;     //当前载入的是那个 ota 扇区
extern struct tm timeinfo;  //当前时间 由sntp 算的
extern int wifi_connect;    //当前是否连了wifi
extern uint8_t cus_strip[4];
extern uint8_t cus_isr[4];
extern char *userid;

#endif