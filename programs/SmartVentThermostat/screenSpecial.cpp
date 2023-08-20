/*
  screenSpecial.cpp - Implement special screen for SmartVent Thermostat.
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
#include <Button_TT_label.h>
#include "nonvolatileSettings.h"
#include "screens.h"
#include "screenSpecial.h"
#include "screenAdvanced.h"
#include "screenCalibration.h"
#include "screenDebug.h"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// SPECIAL SCREEN buttons and fields.
//
// The Special screen shows more buttons to enter additional specialized screens, currently
// the calibration screen and debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Special("SpecialScreen");
static Button_TT_label btn_Calibration("Calibration");
static Button_TT_label btn_Debug("Debug");
static Button_TT_label btn_SpecialDone("SpecialDone");

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Calibration button in Special screen. We switch to Calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Calibration(Button_TT& btn) {
  currentScreen = SCREEN_CALIBRATION;
  drawCalibrationScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Debug button in Special screen. We switch to Debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_Debug(Button_TT& btn) {
  currentScreen = SCREEN_DEBUG;
  drawDebugScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Done button in Special screen. We switch (back) to Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_SpecialDone(Button_TT& btn) {
  currentScreen = SCREEN_ADVANCED;
  drawAdvancedScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initSpecialScreen(void) {
  label_Special.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Special", false, &font18B);

  btn_Calibration.initButton(lcd, "TL", 5, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Calibrate", false, &font12, RAD);
  btn_Debug.initButton(lcd, "TR", 235, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Debug", false, &font12, RAD);

  btn_SpecialDone.initButton(lcd, "BC", 120, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Done", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the special screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawSpecialScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Special.drawButton();

  btn_Calibration.drawButton();
  screenButtons->registerButton(btn_Calibration, btnTap_Calibration);

  btn_Debug.drawButton();
  screenButtons->registerButton(btn_Debug, btnTap_Debug);

  btn_SpecialDone.drawButton();
  screenButtons->registerButton(btn_SpecialDone, btnTap_SpecialDone);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the special screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopSpecialScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
