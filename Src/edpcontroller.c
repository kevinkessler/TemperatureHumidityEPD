/**
 *  @filename   :   edpcontroller.c
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
#include "epdif.h"
#include "epdpaint.h"
#include "imagedata.h"
#include "stm32l0xx_hal.h"
#include "main.h"
#include "displayFunction.h"

#define TEMP_REG 1
#define HUM_REG 2
#define VOLT_REG 3
#define ADC_CAL_REG 4

extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;

unsigned char* frame_buffer;
unsigned char const *nums[10];
unsigned char const *batt_image[5];

int setNumbers(Paint *paint, int16_t value, uint8_t x, uint8_t y, uint8_t symbol) {
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

void drawBattery(Paint *paint, uint8_t value) {
	Paint_DrawBitmapAt(paint, BATT_X, BATT_Y, BATT_W, BATT_H, batt_image[value], COLORED);
}

static uint8_t getBatteryIndex(uint16_t value) {
	if(value > 2900)
		return 4;

	if(value > 2800)
		return 3;

	if(value > 2600)
		return 2;

	if(value > 2400)
		return 1;

	return 0;
}

static void Font_init() {
	//frame_buffer = (unsigned char*)malloc(EPD_WIDTH * EPD_HEIGHT / 8);

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

}

static void enterStandby(uint16_t seconds)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = SPI_CS_Pin|DC_Pin|RST_Pin|BUSY_Pin|EPD_POWER_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, seconds, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

	HAL_PWR_EnterSTANDBYMode();
}

static uint16_t readVoltage(){

	//Wait until ADC is fully powered up, probably not necessary
	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VREFINTRDY));

	HAL_ADC_Start(&hadc);
	HAL_ADC_PollForConversion(&hadc, 1000);

	uint32_t volt_count = HAL_ADC_GetValue(&hadc);

	uint16_t voltage = (3000 * (*VREFINT_CAL_ADDR))/volt_count;
	uint8_t volt_idx = getBatteryIndex(voltage);
	HAL_ADC_Stop(&hadc);

	return volt_idx;
}

static void readSi7021(int16_t *temp, int16_t *hum) {

	uint8_t hum_data[2];
	uint8_t temp_data[2];
	uint8_t cmd;

	cmd=0xF5;   // Read RH, No Clock Stretch
	if(HAL_I2C_Master_Transmit(&hi2c1, 0x80, &cmd, 1,1000) != HAL_OK)
	{
		*temp=999;
		*hum=999;
		return;
	}


	sleepDelay(20);
	uint16_t polls=0;
	do {
		HAL_I2C_Master_Receive(&hi2c1, 0x80, hum_data, 2, 6000);
		if(++polls>1000){
			*temp=999;
			*hum=999;
		}
	} while(hi2c1.ErrorCode & HAL_I2C_ERROR_AF); // Check for NACK


	if(HAL_I2C_Mem_Read(&hi2c1, 0x80, 0xe0, 1, temp_data, 2, 1000) != HAL_OK)    // Read Temperature from Previous Measuement
	{
		*temp=999;
		*hum=999;
		return;
	}


	*hum = (int16_t)(((hum_data[0] * 256 + hum_data[1]) * 125 / 65536.0) - 5.5); //Rounding by adding 0.5
	*temp = (int16_t)(((((temp_data[0] << 8) + temp_data[1]) * 175.72 / 65536.0) - 46.85) * (9.0/5.0) + 32.5);
}

void pollSensors(){



	// Handle cold restart vs STANDY resume
	if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
	{
		//Resumed from STANDBY
		//ADC Must be enabled to set the calibration value
		__HAL_ADC_ENABLE(&hadc);
		uint32_t adc_cal = HAL_RTCEx_BKUPRead(&hrtc,ADC_CAL_REG);
		HAL_ADCEx_Calibration_SetValue(&hadc, ADC_SINGLE_ENDED, adc_cal);
	}
	else
	{
		HAL_ADCEx_Calibration_Start(&hadc,ADC_SINGLE_ENDED);
		uint32_t adc_cal = HAL_ADCEx_Calibration_GetValue(&hadc, ADC_SINGLE_ENDED);
		HAL_RTCEx_BKUPWrite(&hrtc, ADC_CAL_REG, adc_cal);

	}

	Font_init();

	// Configure Standby mode to turn off voltage reference, and not to wait for it to power up after resuming
	HAL_PWREx_EnableFastWakeUp();
	HAL_PWREx_EnableUltraLowPower();

	int16_t temp_last = HAL_RTCEx_BKUPRead(&hrtc, TEMP_REG);
	int16_t hum_last = HAL_RTCEx_BKUPRead(&hrtc, HUM_REG);
	uint8_t volt_last = HAL_RTCEx_BKUPRead(&hrtc, VOLT_REG);

	int16_t hum,temp;

	readSi7021(&temp, &hum);

	uint16_t volt_idx=readVoltage();

	uint8_t sec=20;
	if ((temp_last != temp)||(hum_last != hum)||(volt_last != volt_idx)) {
		sec = 60;
		displayData(temp, hum, volt_idx);
	}

	HAL_RTCEx_BKUPWrite(&hrtc, TEMP_REG, temp);
	HAL_RTCEx_BKUPWrite(&hrtc, HUM_REG, hum);
	HAL_RTCEx_BKUPWrite(&hrtc, VOLT_REG, volt_idx);

	enterStandby(sec);

}

void sleepWait() {
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	HAL_ResumeTick();

}

void sleepDelay(uint16_t delaytime){

	if(delaytime < 10) {
		HAL_Delay(delaytime);
		return;
	}
	else {
		if(HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK) {
			HAL_Delay(delaytime);
			return;
		}
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		uint32_t counter = (uint32_t)(delaytime * 1000) /432;
		if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, (uint16_t)counter, RTC_WAKEUPCLOCK_RTCCLK_DIV16)!=HAL_OK) {
			HAL_Delay(delaytime);
			return;
		}

		HAL_SuspendTick();
		HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
		HAL_ResumeTick();

	}

}

