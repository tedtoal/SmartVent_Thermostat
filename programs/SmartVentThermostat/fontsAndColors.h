/*
  fontsAndColors.h - Define color shortcut symbols and Font_TT objects pointing
  to fonts imported from the GFX font library.
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
#ifndef fontsAndColors_h
#define fontsAndColors_h

#include <Arduino.h>
#include <Font_TT.h>

// Define the colors without the ILI9341_ prefix.
#define BLACK         ILI9341_BLACK
#define NAVY          ILI9341_NAVY
#define DARKGREEN     ILI9341_DARKGREEN
#define DARKCYAN      ILI9341_DARKCYAN
#define MAROON        ILI9341_MAROON
#define PURPLE        ILI9341_PURPLE
#define OLIVE         ILI9341_OLIVE
#define LIGHTGREY     ILI9341_LIGHTGREY
#define DARKGREY      ILI9341_DARKGREY
#define BLUE          ILI9341_BLUE
#define GREEN         ILI9341_GREEN
#define CYAN          ILI9341_CYAN
#define RED           ILI9341_RED
#define MAGENTA       ILI9341_MAGENTA
#define YELLOW        ILI9341_YELLOW
#define WHITE         ILI9341_WHITE
#define ORANGE        ILI9341_ORANGE
#define GREENYELLOW   ILI9341_GREENYELLOW
#define PINK          ILI9341_PINK
#define CLEAR         TRANSPARENT_COLOR

// Pointers to font objects for imported fonts.
extern Font_TT mono12B;
extern Font_TT font9;
extern Font_TT font12;
extern Font_TT font18;
extern Font_TT font24;
extern Font_TT font9B;
extern Font_TT font12B;
extern Font_TT font18B;
extern Font_TT font24B;
//extern Font_TT fontOrg;
//extern Font_TT fontPico;
//extern Font_TT fontTiny;
extern Font_TT fontTom;

#endif // fontsAndColors_h
