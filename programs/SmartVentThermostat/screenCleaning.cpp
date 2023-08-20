/*
  screenCleaning.cpp - Implement cleaning screen for SmartVent Thermostat.
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
#include "screenCleaning.h"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// CLEANING SCREEN buttons and fields.
//
// The Cleaning screen shows a message telling the user he can clean the screen and screen
// presses will be ignored. After he stops touching the screen for a while, it returns to
// the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Cleaning("CleaningScreen");
static Button_TT_label label_CleanTheScreen("CleanTheScreen");
static Button_TT_label label_EndsAfter("EndsAfter");
static Button_TT_label label_NoActivity("NoActivity");

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

// (none)

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the cleaning screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initCleaningScreen(void) {
  label_Cleaning.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Cleaning", false, &font18B);
  label_CleanTheScreen.initButton(lcd, "CC", 120, 100, TEW, TEW, CLEAR, CLEAR, OLIVE,
    "C", "Clean the Screen", false, &font12B);
  label_EndsAfter.initButton(lcd, "TC", 120, 200, TEW, TEW, CLEAR, CLEAR, DARKGREY,
    "C", "Ends After", false, &font12B);
  label_NoActivity.initButton(lcd, "TC", 120, 230, TEW, TEW, CLEAR, CLEAR, DARKGREY,
    "C", "No Activity", false, &font12B);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the cleaning screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawCleaningScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);

  label_Cleaning.drawButton();
  label_CleanTheScreen.drawButton();
  label_EndsAfter.drawButton();
  label_NoActivity.drawButton();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the cleaning screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopCleaningScreen() {
  // No actions required. When LCD backlight timeout occurs, the Main screen will be reactivated.
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
