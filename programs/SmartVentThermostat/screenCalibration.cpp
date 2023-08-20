/*
  screenCalibration.cpp - Implement calibration screen for SmartVent Thermostat.
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
#include "screenCalibration.h"
#include "screenSpecial.h"

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// Length of each arm of "+" sign.
#define PLUS_ARM_LEN 10

// Text for user instructions to tap "+".
#define TEXT_TAP_PLUS "Tap the +"

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// CALIBRATION SCREEN buttons and fields.
//
// The Calibration screen initially shows a Cancel button, a message to touch the "+", and a
// single "+" in one corner. When Cancel is touched the screen exits without changing the
// calibration setting. If the "+" is touched, it is erased and a second "+" in the opposite
// corner is displayed. If that "+" is also touched, it is erased, calibration settings are
// recomputed and temporarily changed, a Save button is shown along with the Cancel button,
// a message is displayed to touch anywhere to test calibration, and subsequent touches cause
// a "+" of another color to be drawn at that position. Touching Cancel reverts to original
// calibration settings, while touching Save saves the new calibration settings in userSettings
// where they will be copied to activeSettings and saved to nonvolatile memory as usual after a
// delay, and the screen exits back to the Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Calibration("CalibrationScreen");
static Button_TT_label label_CalibrationTouch("CalibrationTouch");
static Button_TT_label btn_CalibrationCancel("CalibrationCancel");
static Button_TT_label btn_CalibrationSave("CalibrationSave");

// Display UL and LR calibration positions and corresponding touchscreen calibration coordinates.
static int16_t x_UL, y_UL, x_LR, y_LR;
static int16_t TSx_UL, TSy_UL, TSx_LR, TSy_LR;

// *************************************************************************************** //
// Enums.
// *************************************************************************************** //

// States during calibration and subsequent showing of tapped points.
typedef enum _eCalibState {
  STATE_WAIT_UL,            // Wait for user to tap + at upper-left
  STATE_WAIT_UL_RELEASE,    // Wait for him to release the tap
  STATE_WAIT_LR,            // Wait for user to tap + at lower-right
  STATE_WAIT_LR_RELEASE,    // Wait for him to release the tap
  STATE_WAIT_POINT_SHOW_IT, // Wait for user to tap anywhere, then draw "+" there
  STATE_WAIT_RELEASE        // Wait for him to release the tap
} eCalibState;

// Current state of calibration screen interaction with user.
static eCalibState calibState;

// *************************************************************************************** //
// Local functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw a plus sign at a specified display location.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawPlus(int16_t x, int16_t y, int16_t color, uint8_t len = PLUS_ARM_LEN) {
  lcd->drawFastVLine(x, y-len, 2*len+1, color);
  lcd->drawFastHLine(x-len, y, 2*len+1, color);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Print string S to display at cursor position (x,y) in specified color.
/////////////////////////////////////////////////////////////////////////////////////////////
static void lcd_print(int16_t x, int16_t y, int16_t color, const char* S) {
  lcd->setCursor(x, y);
  lcd->setTextColor(color);
  lcd->print(S);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Calibration screen. We revert calibration parameters to
// the original and switch back to Special screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_CalibrationCancel(Button_TT& btn) {
  ts_display->setTS_calibration(userSettings.TS_LR_X, userSettings.TS_LR_Y,
    userSettings.TS_UL_X, userSettings.TS_UL_Y);
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Calibration screen. We save current calibration parameters
// into "userSettings" and switch back to the Special screen. The values will be copied from
// userSettings to activeSettings sufficient time passes following the last button press.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnTap_CalibrationSave(Button_TT& btn) {
  ts_display->getTS_calibration(&userSettings.TS_LR_X, &userSettings.TS_LR_Y,
    &userSettings.TS_UL_X, &userSettings.TS_UL_Y);
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

// *************************************************************************************** //
// Global functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initCalibrationScreen(void) {
  label_Calibration.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Calibrate", false, &font18B);
  label_CalibrationTouch.initButton(lcd, "TL", 10, 30, 220, TEW, TRANSPARENT_COLOR,
    TRANSPARENT_COLOR, RED, "TL", "Tap the +", false, &font12);

  btn_CalibrationCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_CalibrationSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Calibration screen and register its buttons with the screenButtons
// object. Argument "state" defaults to 1 and is either 1, 2, or 3:
//      1: initial display, draw first +, "tap +", no Save, set STATE_WAIT_UL
//      2: finished STATE_WAIT_UL_RELEASE, draw second +, "tap +", no Save
//      3: finished STATE_WAIT_LR_RELEASE, draw no +, draw Save, "tap to test"
/////////////////////////////////////////////////////////////////////////////////////////////
void drawCalibrationScreen(int state) {

  // Clear all existing button registrations.
  screenButtons->clear();

  // Fill screen with white.
  lcd->fillScreen(ILI9341_WHITE);

  // Get position of the two corner display points at which to draw "+" signs
  // to be tapped.
  ts_display->GetCalibration_UL_LR(PLUS_ARM_LEN+2, &x_UL, &y_UL, &x_LR, &y_LR);

  // Draw and register screen buttons.

  // Calibrate label.
  label_Calibration.drawButton();

  // "Touch ..." instruction label
  label_CalibrationTouch.setLabel(state == 3 ? "Tap to test calibration" : "Tap the +");
  int16_t xL, yT;
  if (state == 1) {
    xL = x_UL;
    yT = y_UL + 2*PLUS_ARM_LEN;
  } else if (state == 2) {
    xL = btn_CalibrationSave.getLeft();
    yT = y_LR - 2*PLUS_ARM_LEN - label_CalibrationTouch.getHeight();
  } else if (state == 3) {
    xL = (lcd->width() - label_CalibrationTouch.getWidth())/2;
    yT = (lcd->height() - label_CalibrationTouch.getHeight())/2;
  }
  label_CalibrationTouch.setPosition(xL, yT);
  label_CalibrationTouch.drawButton();

  // Cancel button.
  btn_CalibrationCancel.drawButton();
  screenButtons->registerButton(btn_CalibrationCancel, btnTap_CalibrationCancel);

  // Save button.
  if (state == 3) {
    btn_CalibrationSave.drawButton();
    screenButtons->registerButton(btn_CalibrationSave, btnTap_CalibrationSave);
  }

  // Draw first or second + or none
  if (state == 1)
    drawPlus(x_UL, y_UL, ILI9341_BLUE);
  else if (state == 2)
    drawPlus(x_LR, y_LR, ILI9341_BLUE);

  // When state = 1, we must initialize calibState because state=1 means the
  // user tapped the "Calibrate" button in the main screen, and we need to
  // restart the calibration state machine.
  if (state == 1)
    calibState = STATE_WAIT_UL;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the calibration screen when it is displayed.
// Note that this functions completely independently and in parallel with the
// processTapsAndReleases() function, which also monitors touch screen actions.
/////////////////////////////////////////////////////////////////////////////////////////////
void loopCalibrationScreen() {
  boolean isTouched = touch->touched();
  TS_Point p;
  if (isTouched)
    p = touch->getPoint();

  switch (calibState) {

  case STATE_WAIT_UL:
    if (isTouched) {
      // Record the touchscreen coordinates.
      TSx_UL = p.x;
      TSy_UL = p.y;
      // Play sound.
      playSound(true);
      calibState = STATE_WAIT_UL_RELEASE;
    }
    break;

  case STATE_WAIT_UL_RELEASE:
    if (!isTouched) {
      // Stop sound.
      playSound(false);
      // Redraw the screen with second +.
      drawCalibrationScreen(2);
      calibState = STATE_WAIT_LR;
    }
    break;

  case STATE_WAIT_LR:
    if (isTouched) {
      // Record the touchscreen coordinates.
      TSx_LR = p.x;
      TSy_LR = p.y;
      // Play sound.
      playSound(true);
      calibState = STATE_WAIT_LR_RELEASE;
    }
    break;

  case STATE_WAIT_LR_RELEASE:
    if (!isTouched) {
      // Stop sound.
      playSound(false);
      // Map the two touchscreen points to the correct calibration values at the
      // extreme ends of the display. Set resulting calibration parameters as the
      // new calibration parameters in ts_display.
      int16_t TS_LR_X, TS_LR_Y, TS_UL_X, TS_UL_Y;
      ts_display->findTS_calibration(x_UL, y_UL, x_LR, y_LR, TSx_UL, TSy_UL, TSx_LR, TSy_LR,
        &TS_LR_X, &TS_LR_Y, &TS_UL_X, &TS_UL_Y);
      ts_display->setTS_calibration(TS_LR_X, TS_LR_Y, TS_UL_X, TS_UL_Y);
      // Redraw the screen with no +.
      drawCalibrationScreen(3);
      calibState = STATE_WAIT_POINT_SHOW_IT;
    }
    break;

  case STATE_WAIT_POINT_SHOW_IT:
    if (isTouched) {
      // Map touched point to display and draw a green "+" at that point.
      int16_t x, y;
      ts_display->mapTStoDisplay(p.x, p.y, &x, &y);
      drawPlus(x, y, DARKGREEN);
      // Play sound.
      playSound(true);
      calibState = STATE_WAIT_RELEASE;
    }
    break;

  case STATE_WAIT_RELEASE:
    if (!isTouched) {
      // Stop sound.
      playSound(false);
      calibState = STATE_WAIT_POINT_SHOW_IT;
    }
    break;

  }
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
