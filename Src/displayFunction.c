/*
 * displayFunction.c
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
#include "epd1in54.h"
#include "main.h"
#include "epdpaint.h"
#include "stm32l0xx_hal.h"
#include "imagedata.h"

EPD epd;


void displayData(int16_t temperature, int16_t humidity, uint8_t battery) {

	GPIO_InitTypeDef GPIO_InitStruct;

	HAL_GPIO_WritePin(EPD_POWER_GPIO_Port, EPD_POWER_Pin, GPIO_PIN_SET);
	/*Configure GPIO pin : BUSY_Pin */
	GPIO_InitStruct.Pin = BUSY_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BUSY_GPIO_Port, &GPIO_InitStruct);

	uint8_t frame_buffer[EPD_WIDTH * EPD_HEIGHT / 8];

	EPD_Init(&epd,lut_full_update);

	Paint paint;
	Paint_Init(&paint, frame_buffer, epd.width, epd.height, ROTATE_90);
	Paint_Clear(&paint, UNCOLORED);

	Paint_DrawBitmapAt(&paint, TEMP_TEXT_X, TEMP_TEXT_Y, TEMP_W, TEMP_H , TEMPERATURE_TEXT, COLORED);
	Paint_DrawBitmapAt(&paint, HUM_TEXT_X, HUM_TEXT_Y, HUM_W, HUM_H , HUMIDITY_TEXT, COLORED);


	setNumbers(&paint, temperature, TEMP_NUM_X, TEMP_NUM_Y, DEGREE_SIGN);

	setNumbers(&paint, humidity, HUM_NUM_X, HUM_NUM_Y, PERCENT_SIGN);

	drawBattery(&paint, battery);


	EPD_SetFrameMemory(&epd, frame_buffer, 0, 0, epd.width, epd.height);
	EPD_DisplayFrame(&epd);

	EPD_WaitUntilIdle(&epd);
	EPD_Sleep(&epd);

	HAL_GPIO_WritePin(EPD_POWER_GPIO_Port, EPD_POWER_Pin, GPIO_PIN_RESET);

}


