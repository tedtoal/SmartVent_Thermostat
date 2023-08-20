/*
  screenDebug.cpp - Implement debug screen for SmartVent Thermostat.
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
#include <floatToString.h>
#include <Button_TT.h>
#include <Button_TT_collection.h>
#include <Button_TT_label.h>
#include "nonvolatileSettings.h"
#include "thermistorAndTemperature.h"
#include "screens.h"
#include "screenDebug.h"
#include "screenSpecial.h"

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

#define DEBUG_ROW_Y_HEIGHT 6 // TomThumb font yAdvance
#define DEBUG_ROW_Y_SPACING (DEBUG_ROW_Y_HEIGHT+2)
#define WIDTH_DEBUG_AREA   220
#define HEIGHT_DEBUG_AREA   220
#define NUM_ROWS_DEBUG_AREA ((uint8_t)(HEIGHT_DEBUG_AREA/DEBUG_ROW_Y_SPACING))
#define SPRINTF_FORMAT_THERMISTOR_R_ROW "%5d in:A=%-5d R=%-6d T=%-4s out:A=%-5d R=%-6d T=%-4s" // 60 + \0
#define LEN_THERMISTOR_R_ROW (5+1+5+5+1+2+6+1+2+4+1+6+5+1+2+6+1+2+4+1) // 60 + \0

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG SCREEN buttons and fields.
//
// The Debug screen shows debug info (currently, indoor temperature resistance values) with a
// "Done" button at the bottom.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Debug("DebugScreen");
static Button_TT_label btn_DebugDone("DebugDone");

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables for managing the fields_DebugArea buttons, and to track the
// position of the next data to be added.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label* fields_DebugArea[NUM_ROWS_DEBUG_AREA];
static uint8_t rowIdx_DebugArea;
static uint16_t lastReadCount_DebugArea;

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if there is a new indoor thermistor R value available, and if so, add it to
// text_DebugArea and set the new label for field_DebugArea and draw the label if changed.
/////////////////////////////////////////////////////////////////////////////////////////////
static void updateDebugScreen() {
  if (lastReadCount_DebugArea != NtempReads) {
    lastReadCount_DebugArea = NtempReads;
    char S[LEN_THERMISTOR_R_ROW];
    char Tin[8];
    char Tout[8];
    floatToString(degCtoF(TlastIndoorTempRead), Tin, sizeof(Tin), 1);
    floatToString(degCtoF(TlastOutdoorTempRead), Tout, sizeof(Tout), 1);
    snprintf_P(S, LEN_THERMISTOR_R_ROW, SPRINTF_FORMAT_THERMISTOR_R_ROW,
      lastReadCount_DebugArea,
      ADClastIndoorTempRead, RlastIndoorTempRead, Tin,
      ADClastOutdoorTempRead, RlastOutdoorTempRead, Tout);
    fields_DebugArea[rowIdx_DebugArea]->setLabelAndDrawIfChanged(S);
    if (++rowIdx_DebugArea == NUM_ROWS_DEBUG_AREA)
      rowIdx_DebugArea = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Done button in Debug screen. We switch to Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_DebugDone(Button_TT& btn) {
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initDebugScreen(void) {
  label_Debug.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Debug", false, &font18B);

  uint16_t y = 40; // Starting y-position
  for (uint8_t i = 0; i < NUM_ROWS_DEBUG_AREA; i++) {
    fields_DebugArea[i] = new Button_TT_label("dbg");
    fields_DebugArea[i]->initButton(lcd, "TL", 10, y, WIDTH_DEBUG_AREA, DEBUG_ROW_Y_HEIGHT, WHITE, WHITE, NAVY,
      "C", "", false, &fontTom);
    y += DEBUG_ROW_Y_SPACING;
  }

  btn_DebugDone.initButton(lcd, "BC", 120, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Done", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the debug screen and register its buttons with the screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawDebugScreen() {
  screenButtons->clear();

  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Debug.drawButton();

  lastReadCount_DebugArea = NtempReads-1;
  rowIdx_DebugArea = 0;
  updateDebugScreen();

  btn_DebugDone.drawButton();
  screenButtons->registerButton(btn_DebugDone, btnTap_DebugDone);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the debug screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopDebugScreen() {
  updateDebugScreen();
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
