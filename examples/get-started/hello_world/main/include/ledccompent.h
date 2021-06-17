#ifndef LEDCCOMPENT_H
#define LEDCCOMPENT_H
#include "application.h"

void ledc_ini(int r, int b, int g, int style, int huang);
void ledc_deini();
void ledc_set_colorful(int color[3]);
void ledc_set_fadtime(int time);
void ledc_change_state(int state);
void ledc_set_lumen(int lu); 
void ledc_set_huang(int strength);
void ledc_color(int setc);
#endif