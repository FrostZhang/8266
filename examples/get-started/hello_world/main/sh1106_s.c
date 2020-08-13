#include <stdio.h>
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "i2c_oled_fonts.h"
#include "sh1106_s.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define ACK_CHECK_EN 0x1			/*!< I2C master will check ack from slave*/
#define SH1106_I2C_ADDRESS 0x78		//0x7a

esp_err_t i2c_cmd(i2c_port_t i2c_num, uint8_t cmdx)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SH1106_I2C_ADDRESS, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //CMD START...
	i2c_master_write_byte(cmd, cmdx, ACK_CHECK_EN); //CMD
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL)
	{
		printf("cmd err:%d\n", ret);
		return ret;
	}
	return ESP_OK;
}
esp_err_t i2c_data(i2c_port_t i2c_num, uint8_t datas)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SH1106_I2C_ADDRESS, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x40, ACK_CHECK_EN); //data cmd contra data
	i2c_master_write_byte(cmd, datas, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL)
	{
		printf("data err:%d\n", ret);
		return ret;
	}
	return ESP_OK;
}
esp_err_t oled_setxy(i2c_port_t i2c_num, uint8_t x, uint8_t y)
{
	uint8_t h = (0xf0 & x) >> 4;
	uint8_t l = (0x0f & x) ; //>> 4 间距过大
	esp_err_t ret = i2c_cmd(i2c_num, 0xb0 | y); //0154 3210
	ret = i2c_cmd(i2c_num, l | 0x01);			//0000 3210
	ret = i2c_cmd(i2c_num, h | 0x10);			//0001 7654
	return ret;
}

esp_err_t oled_cls(i2c_port_t i2c_num)
{
	uint8_t x, y, ret;
	for (y = 0; y < 64; y++)
	{
		if (oled_setxy(i2c_num, 0, y))
			return ESP_OK;
		for (x = 0; x < 132; x++)
		{
			if (i2c_data(i2c_num, 0x00))
				return ESP_OK;
		}
	}
	return ret;
}

void sh1106_init(i2c_port_t i2c_num)
{
	i2c_cmd(i2c_num, 0xAE); //display off
	i2c_cmd(i2c_num, 0x20); //Set Memory Addressing Mode
	i2c_cmd(i2c_num, 0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	i2c_cmd(i2c_num, 0xb0); //Set Page Start Address for Page Addressing Mode,0-7
	i2c_cmd(i2c_num, 0xc8); //Set COM Output Scan Direction
	i2c_cmd(i2c_num, 0x00); //---set low column address
	i2c_cmd(i2c_num, 0x10); //---set high column address
	i2c_cmd(i2c_num, 0x40); //--set start line address
	i2c_cmd(i2c_num, 0x81); //--set contrast control register
	i2c_cmd(i2c_num, 0x7f);
	i2c_cmd(i2c_num, 0xa1); //--set segment re-map 0 to 127
	i2c_cmd(i2c_num, 0xa6); //--set normal display
	i2c_cmd(i2c_num, 0xa8); //--set multiplex ratio(1 to 64)
	i2c_cmd(i2c_num, 0x3F); //
	i2c_cmd(i2c_num, 0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	i2c_cmd(i2c_num, 0xd3); //-set display offset
	i2c_cmd(i2c_num, 0x00); //-not offset
	i2c_cmd(i2c_num, 0xd5); //--set display clock divide ratio/oscillator frequency
	i2c_cmd(i2c_num, 0xf0); //--set divide ratio
	i2c_cmd(i2c_num, 0xd9); //--set pre-charge period
	i2c_cmd(i2c_num, 0x22); //
	i2c_cmd(i2c_num, 0xda); //--set com pins hardware configuration
	i2c_cmd(i2c_num, 0x12);
	i2c_cmd(i2c_num, 0xdb); //--set vcomh
	i2c_cmd(i2c_num, 0x20); //0x20,0.77xVcc
	i2c_cmd(i2c_num, 0x8d); //--set DC-DC enable
	i2c_cmd(i2c_num, 0x14); //
	i2c_cmd(i2c_num, 0xaf); //--turn on oled panel
}

void i2c_on(i2c_port_t i2c_num)
{
	i2c_cmd(i2c_num, 0X8D);
	i2c_cmd(i2c_num, 0X14);
	i2c_cmd(i2c_num, 0XAF);
}

void i2c_off(i2c_port_t i2c_num)
{
	i2c_cmd(i2c_num, 0X8D);
	i2c_cmd(i2c_num, 0X10);
	i2c_cmd(i2c_num, 0XAE);
}

void oled_showStr(unsigned char x, unsigned char y, char ch[], unsigned char TextSize)
{
	unsigned char c = 0, i = 0, j = 0;
	switch (TextSize)
	{
	case 1:
	{
		while (ch[j] != '\0')
		{
			c = ch[j] - 32;
			if (x > 126)
			{
				//next line
				x = 0;
				y++;
			}
			oled_setxy(I2C_MASTER_NUM, x, y);
			for (i = 0; i < 6; i++)
				i2c_data(I2C_MASTER_NUM, F6x8[c*6 + i]);
			x += 6;
			j++;
		}
	}
	break;
	case 2:
	{
		while (ch[j] != '\0')
		{
			printf("%d ", ch[j]);
			c = ch[j] -32;
			if (x > 120)
			{
				x = 0;
				y += 2;
			}
			oled_setxy(I2C_MASTER_NUM, x, y);
			for (i = 0; i < 16; i++)
				//i2c_data(I2C_MASTER_NUM, F8X16[c * 16 + i]);
				i2c_data(I2C_MASTER_NUM, F8X16[c * 32 + i]);
			oled_setxy(I2C_MASTER_NUM, x, y + 1);
			for (i = 0; i < 16; i++)
				//i2c_data(I2C_MASTER_NUM, F8X16[c * 16 + i + 8]);
				i2c_data(I2C_MASTER_NUM, F8X16[c * 32 + i + 16]);
			x += 16;
			j++;
		}
	}
	break;
	}
}

esp_err_t oled_clear()
{
	return oled_cls(I2C_MASTER_NUM);
}

void oled_on()
{
	i2c_on(I2C_MASTER_NUM);
}

void oled_off()
{
	i2c_off(I2C_MASTER_NUM);
}

esp_err_t i2c_testx(i2c_port_t i2c_num)
{
	oled_clear();
	oled_showStr(0, 0, "This is a Test", 2);
	vTaskDelay(1000 / portTICK_RATE_MS);
	return ESP_OK;
}

void i2c_test_task()
{
	esp_err_t ret;
	while (1)
	{
		ret = i2c_testx(I2C_MASTER_NUM);
		if (ret == ESP_OK)
		{
			printf("espok\n");
		}
		else
		{
			printf("No ack, sensor not connected...skip...\n");
		}
	}
	vTaskDelete(NULL);
}

esp_err_t i2c_master_init()
{
	i2c_port_t i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.clk_stretch_tick = I2C_MASTER_FREQ_HZ; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
	//conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	esp_err_t ret = i2c_driver_install(i2c_master_port, conf.mode); //, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0
	i2c_param_config(i2c_master_port, &conf);
	return ret;
}

esp_err_t oled_ini()
{
	esp_err_t err = i2c_master_init();
	if (err == ESP_OK)
	{
		sh1106_init(I2C_MASTER_NUM);
		oled_clear();
	}
	return err;
}
