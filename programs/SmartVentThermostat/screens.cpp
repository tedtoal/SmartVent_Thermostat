/*
  screens.cpp - Definitions used by one or more SmartVent Thermostat screens.
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
#include <Arduino.h>
#include <monitor_printf.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen_TT.h>
#include <TS_Display.h>
#include <Button_TT.h>
#include <Button_TT_collection.h>
#include "pinSettings.h"
#include "screens.h"

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// Frequency to play when user presses the touch screen.
#define TS_TONE_FREQ    3000
// Duty cycle of "tone" (a square wave) in percent.  0 turns it off.
#define TS_TONE_DUTY    50

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

// LCD object.
Adafruit_ILI9341* lcd;

// Touchscreen object.
XPT2046_Touchscreen* touch;

// Touchscreen-LCD object.
TS_Display* ts_display;

// Button collection object to manage the buttons of the currently displayed screen.
Button_TT_collection* screenButtons;

// PWM object for sound from beeper.
SAMD_PWM* sound;

// SmartVent run timer.
uint32_t RunTimeMS;
uint32_t MSatLastRunTimerUpdate;

// *************************************************************************************** //
// Enums.
// *************************************************************************************** //

// SmartVent arm state.
eArmState ArmState;

// Current screen.
eScreen currentScreen;

// *************************************************************************************** //
// Functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize variables used by screens.
/////////////////////////////////////////////////////////////////////////////////////////////
void initScreens() {
  monitor.printf("initScreens()\n");

  // Initialize ArmState OFF.
  monitor.printf("ArmState\n");
  ArmState = ARM_OFF;

  // Initialize SmartVent runtime timer.
  monitor.printf("RunTimeMS\n");
  RunTimeMS = 0;
  MSatLastRunTimerUpdate = millis();

  // Create LCD object, initialize its backlight and timers, and initialize actual displayed data.
  monitor.printf("lcd object\n");
  lcd = new Adafruit_ILI9341(LCD_CS, LCD_DC);
  lcd->begin();
  lcd->setRotation(2);   // portrait mode
  lcd->setTextColor(BLUE);
  lcd->setTextSize(1);
  lcd->setTextWrap(false);

  // Create touchscreen object and initialize it.
  monitor.printf("touch object\n");
  touch = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
  touch->setRotation(lcd->getRotation());
  touch->setThresholds(Z_THRESHOLD/3);
  touch->begin();

  // Create and initialize touchscreen-LCD object.
  monitor.printf("ts_display object\n");
  ts_display = new(TS_Display);
  ts_display->begin(touch, lcd);

  // Create button collection object to manage currently displayed screen buttons.
  monitor.printf("screenButtons object\n");
  screenButtons = new Button_TT_collection;

  // Create PWM object for sound from beeper.
  monitor.printf("sound object\n");
  sound = new SAMD_PWM(BEEPER_PIN, TS_TONE_FREQ, 0);

  monitor.printf("initScreens() done\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set a new value for ArmState.
/////////////////////////////////////////////////////////////////////////////////////////////
void setArmState(eArmState newState) {
  if (ArmState != newState) {
    ArmState = newState;
    monitor.printf("ArmState changed to %d\n", ArmState);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Play (true) or stop playing (false) a sound for touchscreen feedback.
/////////////////////////////////////////////////////////////////////////////////////////////
void playSound(bool play) {
  #ifdef ARDUINO_ARCH_SAMD
  sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, play ? TS_TONE_DUTY : 0);
  #endif
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
