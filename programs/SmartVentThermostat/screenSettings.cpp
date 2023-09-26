/*
  screenSettings.cpp - Implement settings screen for SmartVent Thermostat.
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
#include <Button_TT_arrow.h>
#include <Button_TT_label.h>
#include <Button_TT_uint8.h>
#include <Button_TT_int16.h>
#include "nonvolatileSettings.h"
#include "screens.h"
#include "screenSettings.h"
#include "screenMain.h"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// SETTINGS SCREEN buttons and fields.
//
// The Settings screen shows:
//  -	indoor setpoint temperature for SmartVent
//  - degrees (delta) of difference between indoor temperature and cooler outdoor
//      temperature to turn SmartVent on
//  - degrees (band) of hysteresis, +/- around both the indoor setpoint temperature
//      AND the difference between indoor and outdoor temperature, to turn SmartVent on/off
//  -	run time limit
//  - Cancel button
//  - Save button
//
// The above data is obtained from the userSettings variable. As long as the settings
// screen is active, the current user settings are stored in the button objects.
// When the user exits the Settings screen with the SAVE button, the current button
// object values are copied back to the userSettings variable.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Settings("SettingsScreen");
static Button_TT_label label_TempSetpointOn("Setpoint1");
static Button_TT_int16 field_TempSetpointOn("Setpoint");
static Button_TT_arrow btn_TempSetpointOnLeft("SetpointLeft");
static Button_TT_arrow btn_TempSetpointOnRight("SetpointRight");
static Button_TT_label label1_DeltaTempForOn("DeltaOn1");
static Button_TT_label label2_DeltaTempForOn("DeltaOn2");
static Button_TT_uint8 field_DeltaTempForOn("DeltaOn");
static Button_TT_arrow btn_DeltaTempForOnLeft("DeltaLeft");
static Button_TT_arrow btn_DeltaTempForOnRight("DeltaRight");
static Button_TT_label label_Hysteresis1("Hysteresis1");
static Button_TT_label label_Hysteresis2("Hysteresis2");
static Button_TT_uint8 field_Hysteresis("Hysteresis");
static Button_TT_arrow btn_HysteresisLeft("HysteresisLeft");
static Button_TT_arrow btn_HysteresisRight("HysteresisRight");
static Button_TT_label label_MaxRun1("MaxRun1");
static Button_TT_label label_MaxRun2("MaxRun2");
static Button_TT_uint8 field_MaxRunTime("MaxRunTime");
static Button_TT_arrow btn_MaxRunTimeLeft("MaxRunTimeLeft");
static Button_TT_arrow btn_MaxRunTimeRight("MaxRunTimeRight");
static Button_TT_label label_MaxRun3("MaxRun3");
static Button_TT_label btn_SettingsCancel("SettingsCancel");
static Button_TT_label btn_SettingsSave("SettingsSave");

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the temperature setpoint to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatureSetpoint(bool forceDraw = false) {
  field_TempSetpointOn.setValueAndDrawIfChanged(userSettings.TempSetpointOn, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the fields for the differential temperature settings to current values and draw them.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatureDifferentials(bool forceDraw = false) {
  field_DeltaTempForOn.setValueAndDrawIfChanged(userSettings.DeltaTempForOn, forceDraw);
  field_Hysteresis.setValueAndDrawIfChanged(userSettings.Hysteresis, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the maximum SmartVent run time to the current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showMaxRunTime(bool forceDraw = false) {
  field_MaxRunTime.setValueAndDrawIfChanged(userSettings.MaxRunTimeHours, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Settings screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for temperature setpoint.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_TempSetpointOn(Button_TT& btn) {
  field_TempSetpointOn.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for SmartVent-On temperature differential.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_DeltaTempForOn(Button_TT& btn) {
  field_DeltaTempForOn.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for SmartVent-Off temperature differential.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Hysteresis(Button_TT& btn) {
  field_Hysteresis.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for maximum run time.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_MaxRunTime(Button_TT& btn) {
  field_MaxRunTime.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Settings screen. We switch to Main screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_SettingsCancel(Button_TT& btn) {
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Settings screen. We save settings from the buttons into
// "userSettings" and switch to the Main screen. The values will be copied from userSettings
// to activeSettings after sufficient time passes following the last button press.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_SettingsSave(Button_TT& btn) {
  userSettings.TempSetpointOn = field_TempSetpointOn.getValue();
  userSettings.DeltaTempForOn = field_DeltaTempForOn.getValue();
  userSettings.Hysteresis = field_Hysteresis.getValue();
  const char* pValue = field_MaxRunTime.getLabel();
  if (strcmp(pValue, MAX_RUN_TIME_0) == 0)
    userSettings.MaxRunTimeHours = 0;
  else
    userSettings.MaxRunTimeHours = (uint8_t) atoi(pValue);
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the settings screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initSettingsScreen(void) {
  label_Settings.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Settings", false, &font18B);

  label_TempSetpointOn.initButton(lcd, "TL", 5, 63, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Indoor", false, &font9B);
  field_TempSetpointOn.initButton(lcd, "TR", 150, 59, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_SETPOINT, MAX_TEMP_SETPOINT, true, false);
  btn_TempSetpointOnLeft.initButton(lcd, 'L', "TL", 158, 50, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_TempSetpointOnRight.initButton(lcd, 'R', "TL", 195, 50, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label1_DeltaTempForOn.initButton(lcd, "TL", 5, 101, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Outdoor", false, &font9B);
  label2_DeltaTempForOn.initButton(lcd, "TL", 5, 121, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "lower by", false, &font9B);
  field_DeltaTempForOn.initButton(lcd, "TR", 150, 107, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_DIFFERENTIAL, MAX_TEMP_DIFFERENTIAL, true);
  btn_DeltaTempForOnLeft.initButton(lcd, 'L', "TL", 158, 98, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_DeltaTempForOnRight.initButton(lcd, 'R', "TL", 195, 98, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_Hysteresis1.initButton(lcd, "TL", 5, 148, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Overshoot", false, &font9B);
  label_Hysteresis2.initButton(lcd, "TL", 5, 168, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "+ or -", false, &font9B);
  field_Hysteresis.initButton(lcd, "TR", 150, 155, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_HYSTERESIS, MAX_TEMP_HYSTERESIS, true);
  btn_HysteresisLeft.initButton(lcd, 'L', "TL", 158, 146, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_HysteresisRight.initButton(lcd, 'R', "TL", 195, 146, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_MaxRun1.initButton(lcd, "TL", 5, 216, 50, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Max", false, &font9B);
  label_MaxRun2.initButton(lcd, "TL", 5, 237, 50, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Run", false, &font9B);
  field_MaxRunTime.initButton(lcd, "TL", 57, 222, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, 0, MAX_RUN_TIME_IN_HOURS, false, MAX_RUN_TIME_0);
  btn_MaxRunTimeLeft.initButton(lcd, 'L', "TL", 105, 213, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_MaxRunTimeRight.initButton(lcd, 'R', "TL", 142, 213, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);
  label_MaxRun3.initButton(lcd, "TR", 230, 225, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "hours", false, &font9);

  btn_SettingsCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_SettingsSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the settings screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawSettingsScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Settings.drawButton();

  lcd->drawRoundRect(2, 46, 236, 149, 5, BLACK);

  label_TempSetpointOn.drawButton();
  btn_TempSetpointOnLeft.drawButton();
  btn_TempSetpointOnRight.drawButton();
  screenButtons->registerButton(btn_TempSetpointOnLeft, btnTap_TempSetpointOn);
  screenButtons->registerButton(btn_TempSetpointOnRight, btnTap_TempSetpointOn);
  showTemperatureSetpoint(true);

  label1_DeltaTempForOn.drawButton();
  label2_DeltaTempForOn.drawButton();
  btn_DeltaTempForOnLeft.drawButton();
  btn_DeltaTempForOnRight.drawButton();
  screenButtons->registerButton(btn_DeltaTempForOnLeft, btnTap_DeltaTempForOn);
  screenButtons->registerButton(btn_DeltaTempForOnRight, btnTap_DeltaTempForOn);

  label_Hysteresis1.drawButton();
  label_Hysteresis2.drawButton();
  btn_HysteresisLeft.drawButton();
  btn_HysteresisRight.drawButton();
  screenButtons->registerButton(btn_HysteresisLeft, btnTap_Hysteresis);
  screenButtons->registerButton(btn_HysteresisRight, btnTap_Hysteresis);

  showTemperatureDifferentials(true);

  lcd->drawRoundRect(2, 209, 236, 53, 5, BLACK);

  label_MaxRun1.drawButton();
  label_MaxRun2.drawButton();
  btn_MaxRunTimeLeft.drawButton();
  btn_MaxRunTimeRight.drawButton();
  screenButtons->registerButton(btn_MaxRunTimeLeft, btnTap_MaxRunTime);
  screenButtons->registerButton(btn_MaxRunTimeRight, btnTap_MaxRunTime);
  label_MaxRun3.drawButton();
  showMaxRunTime(true);

  btn_SettingsCancel.drawButton();
  screenButtons->registerButton(btn_SettingsCancel, btnTap_SettingsCancel);
  btn_SettingsSave.drawButton();
  screenButtons->registerButton(btn_SettingsSave, btnTap_SettingsSave);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the settings screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopSettingsScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
