/*
  buttonConstants.h - Constants used in initButton() calls.
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
#ifndef buttonConstants_h
#define buttonConstants_h

// Corner radius of buttons with rounded corners, in pixels.
#define RAD 10

// Somewhat "standard" button width and height.
#define BTN_WIDTH  110
#define BTN_HEIGHT 40

// Negative button width/height values to use when creating buttons for the LCD display.
// A negative width or height means that the button is sized for the initial text label
// supplied for it, with the absolute value of the negative width or height ADDED to the
// text size to get the full button size.
#define ZEW 0   // Zero edge width
#define TEW -1  // Tiny edge width
#define SEW -5  // Small edge width
#define MEW -10 // Medium edge width
#define LEW -20 // Large edge width
#define HEW -30 // Huge edge width

// Values for expU, expD, expL, expR for button constructors.
#define EXP_T 5   // Tiny expansion of button
#define EXP_S 10  // Small expansion of button
#define EXP_M 20  // Medium expansion of button
#define EXP_L 30  // Large expansion of button
#define EXP_H 50  // Huge expansion of button

#endif // buttonConstants_h
