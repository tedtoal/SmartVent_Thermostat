/*
  screenMain.cpp - Implement main screen for SmartVent Thermostat.
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
#include <msToString.h>
#include <Button_TT.h>
#include <Button_TT_collection.h>
#include <Button_TT_label.h>
#include <Button_TT_int16.h>
#include "pinSettings.h"
#include "nonvolatileSettings.h"
#include "thermistorAndTemperature.h"
#include "screens.h"
#include "screenMain.h"
#include "screenAdvanced.h"
#include "screenSettings.h"

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// Strings to show for the arm button when SmartVent is in ON or AUTO mode.
// See showHideSmartVentArmStateButton() for comments about when the arm button
// is shown.
#define STR_ON          "Venting"     // For state ARM_ON
#define STR_ON_TIMEOUT  "Timeout"     // For state ARM_ON_TIMEOUT
#define STR_AUTO_ON     "Venting"     // For state ARM_AUTO_ON
#define STR_AWAIT_HOT   "Wait Hot"    // For state ARM_AWAIT_HOT
#define STR_AWAIT_ON    "Wait On"     // For state ARM_AWAIT_ON

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// MAIN SCREEN buttons and fields.
//
// The Main screen shows:
//  -	indoor and outdoor temperature
//  -	SmartVent status on/off
//  -	SmartVent mode off/auto/on button
//  -	while RunTimeMS is not zero, it is shown
//  - while SmartVent mode is AUTO, the Arm state is shown on a button which, if pressed,
//      cycles the state, allowing the user to either initiate or cancel a wait for the
//      hot part of the day.
//  -	Settings button
//  -	Advanced button
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Smart("Smart");
static Button_TT_label label_Vent("Vent");
static Button_TT_label field_SmartVentOnOff("VentOnOff");
static Button_TT_label btn_OffAutoOn("AutoOnOff");
static Button_TT_int16 field_IndoorTemp("IndoorTemp");
static Button_TT_label label_IndoorTemp("Indoor");
static Button_TT_int16 field_OutdoorTemp("OutdoorTemp");
static Button_TT_label label_OutdoorTemp("Outdoor");
static Button_TT_label field_RunTimer("RunTimer");
static Button_TT_label btn_ArmState("ArmState");
static Button_TT_label btn_Settings("Settings");
static Button_TT_label btn_Advanced("Advanced");

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the fields for the indoor and output temperatures to new current values and draw them.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatures(bool forceDraw = false) {
  field_IndoorTemp.setValueAndDrawIfChanged(
    curIndoorTemperature.Tf_int16+activeSettings.IndoorOffsetF, forceDraw);
  field_OutdoorTemp.setValueAndDrawIfChanged(
    curOutdoorTemperature.Tf_int16+activeSettings.OutdoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for SmartVent ON/OFF to new current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showSmartVentOnOff(bool forceDraw = false) {
  field_SmartVentOnOff.setLabelAndDrawIfChanged(getSmartVent() ? "ON" : "OFF", forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the button label for the SmartVent mode to new active value and draw the button.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showSmartVentModeButton(bool forceDraw = false) {
  const char* S;
  switch(userSettings.SmartVentMode) {
  case MODE_OFF:
    S = "OFF";
    break;
  case MODE_ON:
    S = "ON";
    break;
  case MODE_AUTO:
    S = "AUTO";
    break;
  }
  btn_OffAutoOn.setLabelAndDrawIfChanged(S, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the appropriate label in the RunTimer field.
// If mode is OFF, the RunTimer field is empty, else the RunTimeMS value is shown.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showHideSmartVentRunTimer(bool forceDraw = false) {
  char S[10];
  if (activeSettings.SmartVentMode == MODE_OFF) {
    field_RunTimer.setLabelAndDrawIfChanged("", forceDraw);
  } else {
    msToString(RunTimeMS, S, sizeof(S), true, true, true, 2);
    field_RunTimer.setLabelAndDrawIfChanged(S, forceDraw);
  }
}

// Predeclare button press function used below.
static void btnTap_ArmState(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the appropriate label in the ArmState button and enable or disable the button.
// If SmartVent mode is OFF, the ArmState button is not shown.
// If SmartVent mode is ON or AUTO, the ArmState button is shown.
// When the button is shown, the label reflects the current ArmState value.
// Note: require both the ACTIVE AND USER mode to be equal to each other to show
// the button. Otherwise, things get confusing while user is changing the mode.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showHideSmartVentArmStateButton(bool forceDraw = false) {
  if (activeSettings.SmartVentMode != userSettings.SmartVentMode ||
      activeSettings.SmartVentMode == MODE_OFF) {
    screenButtons->unregisterButton(btn_ArmState);
    btn_ArmState.setOutlineColor(WHITE);
    btn_ArmState.setFillColor(WHITE);
    btn_ArmState.setLabelAndDrawIfChanged("", forceDraw);
  } else {
    btn_ArmState.setOutlineColor(BLACK);
    btn_ArmState.setFillColor(PINK);
    const char* S;
    switch (ArmState) {
    case ARM_ON:
      S = STR_ON;
      break;
    case ARM_ON_TIMEOUT:
      S = STR_ON_TIMEOUT;
      break;
    case ARM_AUTO_ON:
      S = STR_AUTO_ON;
      break;
    case ARM_AWAIT_ON:
      S = STR_AWAIT_ON;
      break;
    case ARM_AWAIT_HOT:
      S = STR_AWAIT_HOT;
      break;
    default:
      S = "Error";
      monitor.printf("ArmState is %d, wrong!\n", ArmState);
      break;
    }
    btn_ArmState.setLabelAndDrawIfChanged(S, forceDraw);
    screenButtons->registerButton(btn_ArmState, btnTap_ArmState);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of SmartVent mode button on Main screen. This cycles the mode: Off -> Auto -> On
// The new value is IMMEDIATELY written to userSettings, there is no "SAVE" button.  It is
// still the case that a delay elapses before userSettings is copied to activeSettings.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_OffAutoOn(Button_TT& btn) {
  Button_TT_label& btnOffAutoOn((Button_TT_label&) btn);
  const char* p = btnOffAutoOn.getLabel();
  eSmartVentMode mode;
  if (strcmp(p, "OFF") == 0) {
    mode = MODE_AUTO;
  } else if (strcmp(p, "AUTO") == 0) {
    mode = MODE_ON;
  } else {
    mode = MODE_OFF;
  }
  userSettings.SmartVentMode = mode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Arm State button on Main screen. The purpose of this button is to allow
// the user to force it out of or in to waiting for HOT or force the run timer back to 0.
//
// This function is only called when the button is visible, which is in these states:
//    ARM_ON_TIMEOUT
//    ARM_AUTO_ON
//    ARM_AWAIT_HOT
//    ARM_AWAIT_ON
//
// In ON mode, state ARM_ON, this clears the run timer.
//
// In ON mode, state ARM_ON_TIMEOUT, this clears the run timer and cycles ArmState back to
// ARM_ON and turns on SmartVent.
//
// In AUTO mode this cycles ArmState between AWAIT_HOT and ARM_AWAIT_ON and clears the run
// timer if coming from state ARM_AWAIT_HOT.
//
// The new arm state takes effect IMMEDIATELY, there is no delay as there is
// with most other setting changes the user makes.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_ArmState(Button_TT& btn) {
  switch (ArmState) {
  case ARM_ON:
    RunTimeMS = 0;
    break;
  case ARM_ON_TIMEOUT:
    RunTimeMS = 0;
    setArmState(ARM_ON);
    setSmartVent(true);
    break;
  case ARM_AUTO_ON:
    setSmartVent(false);
    setArmState(ARM_AWAIT_HOT);
    break;
  case ARM_AWAIT_ON:
    setArmState(ARM_AWAIT_HOT);
    break;
  case ARM_AWAIT_HOT:
    RunTimeMS = 0;
    setArmState(ARM_AWAIT_ON);
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of "Settings" button on Main screen. This switches to the Settings screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Settings(Button_TT& btn) {
  currentScreen = SCREEN_SETTINGS;
  drawSettingsScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of "Advanced" button on Main screen. This switches to the Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Advanced(Button_TT& btn) {
  currentScreen = SCREEN_ADVANCED;
  drawAdvancedScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the main screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initMainScreen(void) {
  label_Smart.initButton(lcd, "TR", 130, 3, SEW, SEW, CLEAR, CLEAR, RED,
    "C", "Smart", false, &font18B);
  label_Vent.initButton(lcd, "TL", 130, 3, SEW, SEW, CLEAR, CLEAR, BLUE,
    "C", "Vent", false, &font18B);

  field_SmartVentOnOff.initButton(lcd, "TL", 10, 50, TEW, TEW, WHITE, WHITE, OLIVE,
    "C", "OFF", false, &font24B);
  btn_OffAutoOn.initButton(lcd, "TR", 230, 45, ZEW, SEW, BLACK, PINK, BLACK,
    "C", "AUTO", false, &font18, RAD, EXP_M, EXP_M, EXP_M, EXP_M);

  field_IndoorTemp.initButton(lcd, "TC", 60, 115, TEW, TEW, WHITE, WHITE, RED,
    "C", &font24B, 0, 0, -99, 199, true);
  label_IndoorTemp.initButton(lcd, "TC", 60, 160, TEW, TEW, CLEAR, CLEAR, RED,
    "C", INDOOR_NAME, false, &font12B);

  field_OutdoorTemp.initButton(lcd, "TC", 175, 115, TEW, TEW, WHITE, WHITE, BLUE,
    "C", &font24B, 0, 0, -99, 199, true);
  label_OutdoorTemp.initButton(lcd, "TC", 175, 160, TEW, TEW, CLEAR, CLEAR, BLUE,
    "C", OUTDOOR_NAME, false, &font12B);

  field_RunTimer.initButton(lcd, "TL", 10, 220, TEW, TEW, WHITE, WHITE, DARKGREEN,
    "C", "00:12:48", false, &mono12B);
  btn_ArmState.initButton(lcd, "TR", 235, 205, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", STR_AWAIT_ON, false, &font12, RAD, EXP_M, EXP_M, 0, 0);

  btn_Settings.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Settings", false, &font12, RAD, EXP_M, EXP_M, 0, 0);
  btn_Advanced.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Advanced", false, &font12, RAD, EXP_M, EXP_M, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the main screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawMainScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);

  label_Smart.drawButton();
  label_Vent.drawButton();

  label_IndoorTemp.drawButton();
  label_OutdoorTemp.drawButton();
  btn_Settings.drawButton();
  screenButtons->registerButton(btn_Settings, btnTap_Settings);
  btn_Advanced.drawButton();
  screenButtons->registerButton(btn_Advanced, btnTap_Advanced);

  showTemperatures(true);
  showSmartVentOnOff(true);
  showSmartVentModeButton(true);
  screenButtons->registerButton(btn_OffAutoOn, btnTap_OffAutoOn);
  showHideSmartVentRunTimer(true);
  showHideSmartVentArmStateButton(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the main screen when it is displayed.
//
// Elements of the main screen that can change are redrawn here (only if they
// have actually changed).
/////////////////////////////////////////////////////////////////////////////////////////////
void loopMainScreen() {
  // Update current indoor and outdoor temperatures on the screen.
  showTemperatures();

  // Update SmartVent ON/OFF on the screen.
  showSmartVentOnOff();

  // Update SmartVent mode button text.
  showSmartVentModeButton();

  // Update SmartVent run timer on the screen.
  showHideSmartVentRunTimer();

  // Update ArmState/DisArmState button on the screen.
  showHideSmartVentArmStateButton();
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
