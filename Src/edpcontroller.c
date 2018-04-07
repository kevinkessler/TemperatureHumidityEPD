/**
 *  @filename   :   imagedata.c
 *  @brief      :   ePaper Display Controller for Temperature/Humidity Display
 *
 * Copyright (C) 2018 Kevin Kessler
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include "epdcontroller.h"
#include "epd1in54b.h"
#include "epdif.h"
#include "epdpaint.h"
#include "imagedata.h"
#include "stm32l0xx_hal.h"

extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c1;

unsigned char* frame_buffer;
unsigned char const *nums[10];
EPD epd;

static int setNumbers(Paint *paint, float value, uint8_t x, uint8_t y, uint8_t symbol) {
	//int display_value = (int)(value + 0.5);
	int display_value = (int)(value);
	if (display_value < 0) {
		if(display_value < -99)
			return -1;
		else
			Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, TEXT_MINUS, COLORED);
	}
	if(display_value > 999)
		return -1;

	uint8_t forceNext =0;
	if(display_value >99){
		uint8_t digit = display_value / 100;
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[digit], COLORED);
		forceNext=1;
		display_value -= digit * 100;
	}

	x += NUMS_W + 4;
	if((display_value > 9) || (forceNext)) {
		uint8_t digit = display_value / 10;
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[digit], COLORED);
		display_value -= digit * 10;
	}
	x += NUMS_W + 4;
	if(display_value < 10)
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[display_value], COLORED);

	x += NUMS_W + 4;
	if (symbol == DEGREE_SIGN)
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, TEXT_DEGREE, COLORED);
	else
		Paint_DrawBitmapAt(paint, x, y, PERC_W, PERC_H, TEXT_PERCENT, COLORED);

	return 0;
}
static void setBattery(Paint *paint, uint8_t value) {
	if(value > 80)
	{
		Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, BATTERY_100, COLORED);
		return;
	}
	if(value > 60)
	{
		Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, BATTERY_75, COLORED);
		return;
	}
	if(value > 40)
	{
		Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, BATTERY_50, COLORED);
		return;
	}
	if(value > 20)
	{
		Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, BATTERY_25, COLORED);
		return;
	}

	Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, BATTERY_0, COLORED);
}
void displayData(float temperature, float humidity, uint8_t battery) {
	   Paint paint_black;
	   Paint_Init(&paint_black, frame_buffer, epd.width, epd.height, ROTATE_90);
	   Paint_Clear(&paint_black, UNCOLORED);

	   Paint_DrawBitmapAt(&paint_black, TEMP_TEXT_X, TEMP_TEXT_Y, TEMP_W, TEMP_H , TEMPERATURE_TEXT, COLORED);
	   Paint_DrawBitmapAt(&paint_black, HUM_TEXT_X, HUM_TEXT_Y, HUM_W, HUM_H , HUMIDITY_TEXT, COLORED);

	   if((temperature <= 78.0) && (temperature >= 68.0))
		   setNumbers(&paint_black, temperature, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN);

	   if((humidity <= 90.0) && (humidity >= 20.0))
		   setNumbers(&paint_black, humidity, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN);

	   if(battery >= 26)
		   setBattery(&paint_black, battery);
	   EPD_Reset(&epd);
	   EPD_Init(&epd);

	   EPD_LoadBlackFrame(&epd, frame_buffer);

	   Paint paint_red;
	   Paint_Init(&paint_red, frame_buffer, epd.width, epd.height, ROTATE_90);

	   Paint_Clear(&paint_red, UNCOLORED);

	   if((temperature > 78.0) || (temperature < 68.0))
		   if(setNumbers(&paint_red, temperature, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN) < 0)
			   setNumbers(&paint_red, 999.0, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN);

	   if((humidity > 90.0) || (humidity < 20.0))
		   if(setNumbers(&paint_red, humidity, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN) < 0)
			   setNumbers(&paint_red, 999.0, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN);

	   if(battery < 26)
		   setBattery(&paint_red, battery);

	   EPD_LoadRedFrame(&epd, frame_buffer);
	   EPD_DisplayRefresh(&epd);
	   EPD_Sleep(&epd);
}

int Display_init() {
	frame_buffer = (unsigned char*)malloc(EPD_WIDTH * EPD_HEIGHT / 8);

	if (EPD_Init(&epd) != 0) {
	    return -1;
	}

	nums[0] = TEXT_0;
	nums[1] = TEXT_1;
	nums[2] = TEXT_2;
	nums[3] = TEXT_3;
	nums[4] = TEXT_4;
	nums[5] = TEXT_5;
	nums[6] = TEXT_6;
	nums[7] = TEXT_7;
	nums[8] = TEXT_8;
	nums[9] = TEXT_9;

	return 0;
}

void pollSensors(){
	  HAL_StatusTypeDef returnValue;
	  uint8_t hum_data[2];
	  uint8_t temp_data[2];

	  __HAL_RCC_I2C1_CLK_ENABLE();
	  returnValue = HAL_I2C_Mem_Read(&hi2c1, 0x80, 0xe5, 1, hum_data, 2, 1000);
	  returnValue = HAL_I2C_Mem_Read(&hi2c1, 0x80, 0xe0, 1, temp_data, 2, 1000);
	  __HAL_RCC_I2C1_CLK_DISABLE();

	  float hum = ((hum_data[0] * 256 + hum_data[1]) * 125 / 65536.0) - 6;
	  float temp = ((((temp_data[0] << 8) + temp_data[1]) * 175.72 / 65536.0) - 46.85) * (9.0/5.0) + 32;

	  returnValue = HAL_ADC_PollForConversion(&hadc, 1000);
	  uint32_t volt_count = HAL_ADC_GetValue(&hadc);

	  float voltage = 3.0 * (*VREFINT_CAL_ADDR)/volt_count;
	  uint8_t v_perc = (uint8_t)(((voltage - 2.0)/1.0) * 100.0);

	  displayData(temp, hum, v_perc);

	  HAL_SuspendTick();
	  __HAL_RCC_PWR_CLK_ENABLE();
	  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
	  HAL_ResumeTick();

}

