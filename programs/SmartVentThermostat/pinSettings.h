/*
  pinSettings.h - Hardware pin definitions and related constants for SmartVent
  Thermostat.
  Created by Ted Toal, 17-Aug-2023
  Released into the public domain.


  Software License Agreement (BSD License)

  Copyright (c) 2023 Ted Toal
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holders nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef pinSettings_h
#define pinSettings_h

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// SmartVent activation relay pin and on/off values.
#define SMARTVENT_RELAY 5
#define SMARTVENT_OFF   LOW
#define SMARTVENT_ON    HIGH

// Beeper pin.
#define BEEPER_PIN        A3

// LCD display pins and parameters.
#define LCD_DC            2
#define LCD_CS            10
#define LCD_MOSI          11
#define LCD_MISO          12
#define LCD_SCLK          13
#define LCD_WIDTH_PIXELS  240
#define LCD_HEIGHT_PIXELS 320

// LCD backlight pin and parameters.
#define LCD_BACKLIGHT_LED A2
#define LCD_BACKLIGHT_OFF HIGH
#define LCD_BACKLIGHT_ON  LOW
#define LCD_BACKLIGHT_AUTO_OFF_MS (30*1000)

// Touchscreen pins.
#define TOUCH_CS          A0
#define TOUCH_IRQ         A7

// ADC, PWM, and AREF pins for ADC calibration.
// Set these pin numbers to valid values for your system. See comments in
// calibSAMD_ADC_withPWM.h describing these pins. These pin numbers are each
// indexes into g_APinDescription[] in variant.cpp. The values shown here are
// my values used on my system:
//  PIN_ADC_CALIB   7 = D7 = PA06 = AIN6
//  PIN_PWM_CALIB   4 = D4 = PA07 = TCC1 ch 1
//  PIN_AREF_OUT    6 = D6 = PA04
#define PIN_ADC_CALIB 7
#define PIN_PWM_CALIB 4
#define PIN_AREF_OUT  6

// Set this to 0 to disable ADC multiple sampling and averaging, or a number X
// between 1 and 10 to average 2^X samples, e.g. 6 means average 2^6 = 64 samples.
// This is internal hardware-based averaging. It produces more stable ADC values.
#define CFG_ADC_MULT_SAMP_AVG 6

// *************************************************************************************** //
// Functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize pins not initialized elsewhere. Currently initialized:
//  - SMARTVENT_RELAY (initialized off)
//  - Backlight (initialized on)
/////////////////////////////////////////////////////////////////////////////////////////////
extern void initPins();

/////////////////////////////////////////////////////////////////////////////////////////////
// Get the display backlight state, true if on, false if off.
/////////////////////////////////////////////////////////////////////////////////////////////
extern bool getBacklight();

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the display backlight on or off.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void setBacklight(bool on);

/////////////////////////////////////////////////////////////////////////////////////////////
// Get the SmartVent relay state, true if on, false if off.
/////////////////////////////////////////////////////////////////////////////////////////////
extern bool getSmartVent();

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the SmartVent relay on or off.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void setSmartVent(bool on);

#endif // pinSettings_h
