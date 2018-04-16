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
#include "main.h"

#define TEMP_REG 1
#define HUM_REG 2
#define VOLT_REG 3

extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;

unsigned char* frame_buffer;
unsigned char const *nums[10];
unsigned char const *batt_image[5];

EPD epd;

static int setNumbers(Paint *paint, int16_t value, uint8_t x, uint8_t y, uint8_t symbol) {
	if (value < 0) {
		if(value < -99)
			return -1;
		else
			Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, TEXT_MINUS, COLORED);
	}
	if(value > 999)
		return -1;

	uint8_t forceNext =0;
	if(value >99){
		uint8_t digit = value / 100;
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[digit], COLORED);
		forceNext=1;
		value -= digit * 100;
	}

	x += NUMS_W + 4;
	if((value > 9) || (forceNext)) {
		uint8_t digit = value / 10;
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[digit], COLORED);
		value -= digit * 10;
	}
	x += NUMS_W + 4;
	if(value < 10)
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, nums[value], COLORED);

	x += NUMS_W + 4;
	if (symbol == DEGREE_SIGN)
		Paint_DrawBitmapAt(paint, x, y, NUMS_W, NUMS_H, TEXT_DEGREE, COLORED);
	else
		Paint_DrawBitmapAt(paint, x, y, PERC_W, PERC_H, TEXT_PERCENT, COLORED);

	return 0;
}

static void drawBattery(Paint *paint, uint8_t value) {
	Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, batt_image[value], COLORED);
}

static uint8_t getBatteryIndex(uint8_t value) {
	if(value > 80)
		return 4;

	if(value > 60)
		return 3;

	if(value > 40)
		return 2;

	if(value > 20)
		return 1;

	return 0;
}

void displayData(int16_t temperature, int16_t humidity, uint8_t battery) {

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

int Display_init() {
	frame_buffer = (unsigned char*)malloc(EPD_WIDTH * EPD_HEIGHT / 8);

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

	batt_image[0] = BATTERY_0;
	batt_image[1] = BATTERY_25;
	batt_image[2] = BATTERY_50;
	batt_image[3] = BATTERY_75;
	batt_image[4] = BATTERY_100;

	return 0;
}

void enterStandby()
{
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = SPI_CS_Pin|DC_Pin|RST_Pin|BUSY_Pin|EPD_POWER_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 20, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

	HAL_PWR_EnterSTANDBYMode();
}

void pollSensors(){

	uint8_t hum_data[2];
	uint8_t temp_data[2];

	HAL_I2C_Mem_Read(&hi2c1, 0x80, 0xe5, 1, hum_data, 2, 1000);
	HAL_I2C_Mem_Read(&hi2c1, 0x80, 0xe0, 1, temp_data, 2, 1000);

	int16_t hum = (int16_t)(((hum_data[0] * 256 + hum_data[1]) * 125 / 65536.0) - 5.5); //Rounding by adding 0.5
	int16_t temp = (int16_t)(((((temp_data[0] << 8) + temp_data[1]) * 175.72 / 65536.0) - 46.85) * (9.0/5.0) + 32.5);


	HAL_ADC_PollForConversion(&hadc, 1000);
	uint32_t volt_count = HAL_ADC_GetValue(&hadc);

	float voltage = 3.0 * (*VREFINT_CAL_ADDR)/volt_count;
	uint8_t v_perc = (uint8_t)(((voltage - 2.0)/1.0) * 100.0);
	uint8_t volt_idx = getBatteryIndex(v_perc);

	displayData(temp, hum, volt_idx);
	enterStandby();


}

