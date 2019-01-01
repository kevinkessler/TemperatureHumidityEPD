/*
 * epdcontroller.h
 *
 *  Created on: Apr 2, 2018
 *      Author: kevin
 */

#ifndef INC_EPDCONTROLLER_H_
#define INC_EPDCONTROLLER_H_

#include "epdpaint.h"

#define VREFINT_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FF80078))

#define COLORED      1
#define UNCOLORED    0

#define DEGREE_SIGN 0
#define PERCENT_SIGN 1

#define TEMP_TEXT_X 19
#define TEMP_TEXT_Y 18
#define TEMP_NUM_X 43
#define TEMP_NUM_Y 49

#define HUM_TEXT_X 19
#define HUM_TEXT_Y 114
#define HUM_NUM_X 43
#define HUM_NUM_Y 147

#define BATT_X 172
#define BATT_Y 2

#define TEMP_REG 1
#define HUM_REG 2
#define VOLT_REG 3
#define ADC_CAL_REG 4
#define COUNT_REG 5

void drawBattery(Paint *paint, uint8_t value);
int setNumbers(Paint *paint, int16_t value, uint8_t x, uint8_t y, uint8_t symbol);
void pollSensors(void);
void sleepDelay(uint16_t delaytime);
void sleepWait(void);

#endif /* INC_EPDCONTROLLER_H_ */
