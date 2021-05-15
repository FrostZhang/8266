
#ifndef SH1106_H
#define SH1106_H

#include <stdio.h>

#define I2C_MASTER_SCL_IO 14		/*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 02		/*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 74800	/*!< I2C master clock frequency,<400 khz*/

esp_err_t oled_ini();
esp_err_t oled_clear();
void oled_showStr(unsigned char x, unsigned char y, char ch[], unsigned char TextSize);

void oled_off();

void oled_on();

#endif /* SH1106_H */