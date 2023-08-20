/*
  screenCalibration.h - Definitions for SmartVent Thermostat calibration screen.
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
#ifndef screenCalibration_h
#define screenCalibration_h

// *************************************************************************************** //
// Functions.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void initCalibrationScreen();

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the calibration screen and register its buttons with the screenButtons object.
// The "state" argument must be 1, 2, or 3:
//  1: initial call when screen first entered, first "+" is drawn
//  2: call to redraw screen with second "+"
//  3: call to redraw screen with no "+"
/////////////////////////////////////////////////////////////////////////////////////////////
void drawCalibrationScreen(int state=1);

/////////////////////////////////////////////////////////////////////////////////////////////
// Perform loop() function processing for the calibration screen when it is displayed.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void loopCalibrationScreen();

#endif // screenCalibration_h
