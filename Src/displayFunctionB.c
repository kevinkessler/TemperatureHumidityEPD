/*
 * displayFunctionB.c
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
#include "displayFunction.h"
#include "epdcontroller.h"
#include "epd1in54b.h"
#include "main.h"
#include "epdpaint.h"
#include "stm32l0xx_hal.h"
#include "imagedata.h"

EPD epd;

void displayData(int16_t temperature, int16_t humidity, uint8_t battery) {

	uint8_t frame_buffer[EPD_WIDTH * EPD_HEIGHT / 8];

	HAL_GPIO_WritePin(EPD_POWER_GPIO_Port, EPD_POWER_Pin, GPIO_PIN_RESET);
	EPD_Init(&epd);

	Paint paint_black;
	Paint_Init(&paint_black, frame_buffer, epd.width, epd.height, ROTATE_90);
	Paint_Clear(&paint_black, UNCOLORED);

	Paint_DrawBitmapAt(&paint_black, TEMP_TEXT_X, TEMP_TEXT_Y, TEMP_W, TEMP_H , TEMPERATURE_TEXT, COLORED);
	Paint_DrawBitmapAt(&paint_black, HUM_TEXT_X, HUM_TEXT_Y, HUM_W, HUM_H , HUMIDITY_TEXT, COLORED);


	if((temperature <= 78) && (temperature >= 60))
	   setNumbers(&paint_black, temperature, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN);

	if((humidity <= 90) && (humidity >= 20))
	   setNumbers(&paint_black, humidity, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN);

	if(battery >= 2)
	   drawBattery(&paint_black, battery);


	EPD_LoadBlackFrame(&epd, frame_buffer);

	Paint paint_red;
	Paint_Init(&paint_red, frame_buffer, epd.width, epd.height, ROTATE_90);

	Paint_Clear(&paint_red, UNCOLORED);

	if((temperature > 78) || (temperature < 60))
	   if(setNumbers(&paint_red, temperature, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN) < 0)
		   setNumbers(&paint_red, 999, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN);

	if((humidity > 90) || (humidity < 20))
	   if(setNumbers(&paint_red, humidity, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN) < 0)
		   setNumbers(&paint_red, 999, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN);

	if(battery < 2)
	   drawBattery(&paint_red, battery);

	EPD_LoadRedFrame(&epd, frame_buffer);


	EPD_DisplayRefresh(&epd);
	EPD_WaitUntilIdle(&epd);
	EPD_Sleep(&epd);
	HAL_GPIO_WritePin(EPD_POWER_GPIO_Port, EPD_POWER_Pin, GPIO_PIN_SET);

}
