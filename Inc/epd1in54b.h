/**
 *  @filename   :   epd1in54b.h
 *  @brief      :   Header file for Dual-color e-paper library epd1in54b.c
 *  @author     :   Yehui from Waveshare
 *  
 *  Copyright (C) Waveshare     August 2 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EPD1IN54B_H
#define EPD1IN54B_H

//#include "fonts.h"

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200

// EPD1IN54B commands
#define PANEL_SETTING                               0x00
#define POWER_SETTING                               0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define POWER_ON_MEASURE                            0x05
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP                                  0x07
#define DATA_START_TRANSMISSION_1                   0x10
#define DATA_STOP                                   0x11
#define DISPLAY_REFRESH                             0x12
#define DATA_START_TRANSMISSION_2                   0x13
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_SENSOR_COMMAND                  0x40
#define TEMPERATURE_SENSOR_CALIBRATION              0x41
#define TEMPERATURE_SENSOR_WRITE                    0x42
#define TEMPERATURE_SENSOR_READ                     0x43
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define LOW_POWER_DETECTION                         0x51
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define SOURCE_AND_GATE_START_SETTING               0x62
#define GET_STATUS                                  0x71
#define AUTO_MEASURE_VCOM                           0x80
#define VCOM_VALUE                                  0x81
#define VCM_DC_SETTING_REGISTER                     0x82
#define PROGRAM_MODE                                0xA0
#define ACTIVE_PROGRAM                              0xA1
#define READ_OTP_DATA                               0xA2

extern const unsigned char lut_vcom0[];
extern const unsigned char lut_vcom1[];
extern const unsigned char lut_w[];
extern const unsigned char lut_b[];
extern const unsigned char lut_g1[];
extern const unsigned char lut_g2[];
extern const unsigned char lut_red0[];
extern const unsigned char lut_red1[];

typedef struct EPD_t {
  int reset_pin;
  int dc_pin;
  int cs_pin;
  int busy_pin;
  int width;
  int height;
} EPD;

/* Hardware operating functions */
int  EPD_Init(EPD* epd);
void EPD_WaitUntilIdle(EPD* epd);
void EPD_DelayMs(EPD* epd, unsigned int delay_time);
void EPD_Reset(EPD* epd);
void EPD_SetLutBw(EPD* epd);
void EPD_SetLutRed(EPD* epd);
void EPD_DisplayFrame(EPD* epd, const unsigned char* frame_buffer_black, const unsigned char* frame_buffer_red);
void EPD_Sleep(EPD* epd);
void EPD_DigitalWrite(EPD* epd, int pin, int value);
int  EPD_DigitalRead(EPD* epd, int pin);
void EPD_SendCommand(EPD* epd, unsigned char command);
void EPD_SendData(EPD* epd, unsigned char data);
void EPD_LoadBlackFrame(EPD *epd, const unsigned char* frame_buffer);
void EPD_LoadRedFrame(EPD *epd, const unsigned char* frame_buffer);
void EPD_DisplayRefresh(EPD *epd);

#endif /* EPD1IN54B_H */

/* END OF FILE */
