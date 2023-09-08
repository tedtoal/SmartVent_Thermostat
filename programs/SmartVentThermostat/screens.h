/*
  screens.h - Declarations and definitions used by one or more SmartVent
  Thermostat screens.
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
#ifndef screens_h
#define screens_h

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen_TT.h>
#include <TS_Display.h>
#include <Button_TT.h>
#include <Button_TT_collection.h>
#include "buttonConstants.h"
#include "fontsAndColors.h"

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// Set this to 1 to enable debug output to the serial monitor.
// Set this to 0 when releasing code to use without the IDE, USB port, and serial monitor.
// ****** IF THERMOSTAT WON'T DISPLAY ANYTHING, DID YOU SET THIS TO 0 IF NO MONITOR PORT? ******
#define USE_MONITOR_PORT 0

// Names to use on the display for indoors and outdoors.
#define INDOOR_NAME "Indoor"
#define OUTDOOR_NAME "Outdoor"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

// LCD object.
extern Adafruit_ILI9341* lcd;

// Touchscreen object.
extern XPT2046_Touchscreen* touch;

// Touchscreen-LCD object.
extern TS_Display* ts_display;

// Button collection object to manage the buttons of the currently displayed screen.
extern Button_TT_collection* screenButtons;

// SmartVent run timer. This counts by milliseconds whenever SmartVent is on (via
// ON or AUTO mode). The count is clamped at a maximum of 99 hours, which should
// only happen if mode is ON and SmartVent is allowed to run for over 4 days straight.
// This resets back to zero when SmartVent mode is OFF or when the user's maximum
// SmartVent run time is reached in AUTO or ON mode.
extern uint32_t RunTimeMS;

// In order to be able to update the value of RunTimeMS periodically, the
// following variable records the milliseconds time (millis()) at which the last
// update was made to it, so that when the next update time occurs, the number
// of elapsed milliseconds can be determined and used to do the update.
extern uint32_t MSatLastRunTimerUpdate;

// *************************************************************************************** //
// Enums.
// *************************************************************************************** //

// Arm states for SmartVent AUTO mode.
//  ARM_OFF: whenever SmartVent mode is OFF.
//  ARM_ON: whenever SmartVent mode is ON and the run timer has not timed out.
//  ARM_ON_TIMEOUT: whenever SmartVent mode is ON and the run timer HAS timed out.
//  ARM_AWAIT_ON: initial arming state when the AUTO mode is activated (which can
//    occur simply by cycling modes through OFF, ON, AUTO, but may change immediately
//    to ARM_AUTO_ON if the outdoor temperature is low enough). This state also
//    occurs when state is ARM_AWAIT_HOT and outdoor temperature is hot enough.
//  ARM_AUTO_ON: occurs when state is ARM_AWAIT_ON and indoor temperature becomes
//    >= SmartVent setpoint temperature and outdoor temperature becomes <= indoor
//    temperature - DeltaTempForOn. At that time, SmartVent is turned ON and a
//    run-time timer is started.
//  ARM_AWAIT_HOT occurs when the state is ARM_AUTO_ON and a maximum SmartVent
//    run time is set and the SmartVent run timer reaches that value. At that time,
//    SmartVent is turned off. Exit from this state to ARM_AWAIT_ON when outdoor
//    temperature becomes >= indoor temperature + DeltaNewDayTemp.
typedef enum _eArmState {
  ARM_OFF,          // SmartVent is off in OFF mode.
  ARM_ON,           // SmartVent is on in ON mode.
  ARM_ON_TIMEOUT,   // SmartVent is off in ON mode.
  ARM_AUTO_ON,      // SmartVent is on in AUTO mode
  ARM_AWAIT_HOT,    // Timed out running in auto, SmartVent now off in AUTO mode and waiting till outdoor temp >= indoor temp + DeltaNewDayTemp
  ARM_AWAIT_ON      // SmartVent now off in AUTO mode and waiting till indoor temp >= setpoint temp and outdoor temp <= indoor temp - DeltaTempForOn
} eArmState;

// SmartVent arm state.
extern eArmState ArmState;

// Screens.
typedef enum _eScreen {
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_ADVANCED,
  SCREEN_CLEANING,
  SCREEN_SPECIAL,
  SCREEN_CALIBRATION,
  SCREEN_DEBUG
} eScreen;

// Current screen.
extern eScreen currentScreen;

// *************************************************************************************** //
// Functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize variables used by screens.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void initScreens();

/////////////////////////////////////////////////////////////////////////////////////////////
// Set a new value for ArmState.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void setArmState(eArmState newState);

/////////////////////////////////////////////////////////////////////////////////////////////
// Play (true) or stop playing (false) a sound for touchscreen feedback.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void playSound(bool play);

#endif // screens_h
