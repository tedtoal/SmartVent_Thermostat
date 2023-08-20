/*
  screenAdvanced.cpp - Implement advanced screen for SmartVent Thermostat.
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
#include <Button_TT_int8.h>
#include "nonvolatileSettings.h"
#include "screens.h"
#include "screenAdvanced.h"
#include "screenCleaning.h"
#include "screenMain.h"
#include "screenSpecial.h"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// ADVANCED SCREEN buttons and fields.
//
// The Advanced screen shows:
//  - delta arm temperature for rearming
//  -	calibration delta temperatures for indoors and outdoors
//  - Cleaning button
//  - Special button
//  - Cancel button
//  - Save button
//
// The delta arm temperature and calibration delta temperatures are obtained from the
// userSettings variable. As long as the Advanced screen is active, the current user settings
// are stored in the button objects. When the user exits the Advanced screen with the SAVE
// button, the current button object values are copied back to the userSettings variable.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Advanced("AdvancedScreen");
static Button_TT_label label_DeltaNewDayTemp("DeltaArm1");
static Button_TT_uint8 field_DeltaNewDayTemp("DeltaArm");
static Button_TT_arrow btn_DeltaNewDayTempLeft("DeltaArmLeft");
static Button_TT_arrow btn_DeltaNewDayTempRight("DeltaArmRight");
static Button_TT_label label_IndoorOffset1("IndoorOffset1");
static Button_TT_label label_IndoorOffset2("IndoorOffset2");
static Button_TT_int8 field_IndoorOffset("IndoorOffset");
static Button_TT_arrow btn_IndoorOffsetLeft("IndoorLeft");
static Button_TT_arrow btn_IndoorOffsetRight("IndoorRight");
static Button_TT_label label_Outdoor("OutdoorOffset1");
static Button_TT_label label_OutdoorOffset("OutdoorOffset2");
static Button_TT_int8 field_OutdoorOffset("OutdoorOffset");
static Button_TT_arrow btn_OutdoorOffsetLeft("OutdoorOffsetLeft");
static Button_TT_arrow btn_OutdoorOffsetRight("OutdoorOffsetRight");
static Button_TT_label btn_Cleaning("Cleaning");
static Button_TT_label btn_Special("Special");
static Button_TT_label btn_AdvancedCancel("AdvancedCancel");
static Button_TT_label btn_AdvancedSave("AdvancedSave");

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the delta arm temperature to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showDeltaNewDayTemp(bool forceDraw = false) {
  field_DeltaNewDayTemp.setValueAndDrawIfChanged(userSettings.DeltaNewDayTemp, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the indoor offset to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showIndoorOffset(bool forceDraw = false) {
  field_IndoorOffset.setValueAndDrawIfChanged(userSettings.IndoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the outdoor offset to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showOutdoorOffset(bool forceDraw = false) {
  field_OutdoorOffset.setValueAndDrawIfChanged(userSettings.OutdoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for delta arm temperature.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_DeltaNewDayTemp(Button_TT& btn) {
  field_DeltaNewDayTemp.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for indoor offset.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_IndoorOffset(Button_TT& btn) {
  field_IndoorOffset.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for outdoor offset.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_OutdoorOffset(Button_TT& btn) {
  field_OutdoorOffset.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cleaning button in Advanced screen. We switch to Cleaning screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Cleaning(Button_TT& btn) {
  currentScreen = SCREEN_CLEANING;
  drawCleaningScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Special button in Advanced screen. We switch to Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Special(Button_TT& btn) {
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Advanced screen. We switch to Main screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_AdvancedCancel(Button_TT& btn) {
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Advanced screen. We save settings from the buttons into
// "userSettings" AND INTO "activeSettings", so they take effect immediately, then switch to
// the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_AdvancedSave(Button_TT& btn) {
  userSettings.DeltaNewDayTemp = activeSettings.DeltaNewDayTemp = field_DeltaNewDayTemp.getValue();
  userSettings.IndoorOffsetF = activeSettings.IndoorOffsetF = field_IndoorOffset.getValue();
  userSettings.OutdoorOffsetF = activeSettings.OutdoorOffsetF = field_OutdoorOffset.getValue();
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initAdvancedScreen(void) {
  label_Advanced.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Advanced", false, &font18B);

  label_DeltaNewDayTemp.initButton(lcd, "TL", 5, 63, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Arm Diff", false, &font9B);
  field_DeltaNewDayTemp.initButton(lcd, "TL", 90, 59, TEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, 1, MAX_DELTA_ARM_TEMP, true);
  btn_DeltaNewDayTempLeft.initButton(lcd, 'L', "TL", 158, 50, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_DeltaNewDayTempRight.initButton(lcd, 'R', "TL", 195, 50, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_IndoorOffset1.initButton(lcd, "TL", 5, 117, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Indoor", false, &font9B);
  label_IndoorOffset2.initButton(lcd, "TL", 5, 138, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "offset", false, &font9B);
  field_IndoorOffset.initButton(lcd, "TL", 90, 123, SEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, -MAX_TEMP_CALIB_DELTA, MAX_TEMP_CALIB_DELTA, true, true);
  btn_IndoorOffsetLeft.initButton(lcd, 'L', "TL", 158, 114, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_IndoorOffsetRight.initButton(lcd, 'R', "TL", 195, 114, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_Outdoor.initButton(lcd, "TL", 5, 165, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Outdoor", false, &font9B);
  label_OutdoorOffset.initButton(lcd, "TL", 5, 186, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "offset", false, &font9B);
  field_OutdoorOffset.initButton(lcd, "TL", 90, 171, SEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, -MAX_TEMP_CALIB_DELTA, MAX_TEMP_CALIB_DELTA, true, true);
  btn_OutdoorOffsetLeft.initButton(lcd, 'L', "TL", 158, 162, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_OutdoorOffsetRight.initButton(lcd, 'R', "TL", 195, 162, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  btn_Cleaning.initButton(lcd, "TL", 5, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cleaning", false, &font12, RAD);
  btn_Special.initButton(lcd, "TR", 235, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Special", false, &font12, RAD);

  btn_AdvancedCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_AdvancedSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the advanced screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawAdvancedScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Advanced.drawButton();

  lcd->drawRoundRect(2, 46, 236, 53, 5, BLACK);

  label_DeltaNewDayTemp.drawButton();
  btn_DeltaNewDayTempLeft.drawButton();
  btn_DeltaNewDayTempRight.drawButton();
  screenButtons->registerButton(btn_DeltaNewDayTempLeft, btnTap_DeltaNewDayTemp);
  screenButtons->registerButton(btn_DeltaNewDayTempRight, btnTap_DeltaNewDayTemp);
  showDeltaNewDayTemp(true);

  lcd->drawRoundRect(2, 110, 236, 102, 5, BLACK);

  label_IndoorOffset1.drawButton();
  label_IndoorOffset2.drawButton();
  btn_IndoorOffsetLeft.drawButton();
  btn_IndoorOffsetRight.drawButton();
  screenButtons->registerButton(btn_IndoorOffsetLeft, btnTap_IndoorOffset);
  screenButtons->registerButton(btn_IndoorOffsetRight, btnTap_IndoorOffset);
  showIndoorOffset(true);

  label_Outdoor.drawButton();
  label_OutdoorOffset.drawButton();
  btn_OutdoorOffsetLeft.drawButton();
  btn_OutdoorOffsetRight.drawButton();
  screenButtons->registerButton(btn_OutdoorOffsetLeft, btnTap_OutdoorOffset);
  screenButtons->registerButton(btn_OutdoorOffsetRight, btnTap_OutdoorOffset);
  showOutdoorOffset(true);

  btn_Cleaning.drawButton();
  screenButtons->registerButton(btn_Cleaning, btnTap_Cleaning);

  btn_Special.drawButton();
  screenButtons->registerButton(btn_Special, btnTap_Special);

  btn_AdvancedCancel.drawButton();
  screenButtons->registerButton(btn_AdvancedCancel, btnTap_AdvancedCancel);
  btn_AdvancedSave.drawButton();
  screenButtons->registerButton(btn_AdvancedSave, btnTap_AdvancedSave);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the advanced screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopAdvancedScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
