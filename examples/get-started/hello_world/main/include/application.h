#ifndef APPLICATION_H
#define APPLICATION_H
#include "sb.h"

void http_url_encode(char url[]);
void http_url_decode(char url[]);
void system_restart();
void system_pilot_light(int is_on);
void print_free_heap_size();
#endif