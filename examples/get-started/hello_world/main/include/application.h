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

#define ON 0
#define OPEN 0
#define OFF 1
#define CLOSE 1 

// #define APP_STRIP_4  1  //插排 4 孔
// #define APP_STRIP_3  0  //插排 3 孔
// #define APP_IR 0  //红外 
#define APP_LEDC 1  //ledc 

#if defined(APP_LEDC)
#define LEDC_IO_NUM0 GPIO_NUM_12
#define LEDC_IO_NUM1 GPIO_NUM_15
#define LEDC_IO_NUM2 GPIO_NUM_14
#define LEDC_IO_NUM3 GPIO_NUM_13
#endif

void http_url_encode(char url[]);
void http_url_decode(char url[]);
void system_restart();
void system_pilot_light(int is_on);
void print_free_heap_size();
int strSearch(char *str1, char *str2);
char *substring(char *src, int pos, int length);

extern char* XINHAO;        //型号
extern char* OTA_LABLE;     //当前载入的是那个 ota 扇区
extern struct tm timeinfo;  //当前时间 由sntp 算的
extern int wifi_connect;    //当前是否连了wifi
extern uint8_t cus_strip[4];
extern uint8_t cus_isr[4];
extern char *userid;
extern int gpio_bit; 

#endif