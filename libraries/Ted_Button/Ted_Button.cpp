/*
  This was created by edits to the Adafruit_GFX_Button class in the library
  Adafruit_GFX_Library.
*/

#include "Ted_Button.h"
#include "monitor_printf.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

// Following stuff is borrowed from Adafruit_GFX.cpp
#include "glcdfont.c"
#ifdef __AVR__
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#endif

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
  return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
  // expression in __AVR__ section may generate "dereferencing type-punned
  // pointer will break strict-aliasing rules" warning In fact, on other
  // platforms (such as STM32) there is no need to do this pointer magic as
  // program memory may be read in a usual way So expression may be simplified
  return gfxFont->glyph + c;
#endif //__AVR__
}

/**************************************************************************/
void Ted_Button::getLabelBounds(const char *str, int16_t& dX, int16_t& dY,
                                uint16_t& w, uint16_t& h,
                                int16_t& dXcF) {

  _gfx->getTextBounds(str, 0, 0, &dX, &dY, &w, &h, &dXcF);

  // Add one pixel on each side of the text bounding box to account for
  // apparent inaccuracy creeping in somewhere.
  dX -= 1;
  dY -= 1;
  w += 2;
  h += 2;
  dXcF += 1;
  //monitor_printf("name:%s  getLabelBounds(\"%s\"): dX:%d dY:%d w:%d h:%d dXcF:%d\n",
  //  _name, str, dX, dY, w, h, dXcF);
}

/**************************************************************************/
uint16_t Ted_Button::digitWidths[10];
bool Ted_Button::haveDigitWidths = false;
uint8_t Ted_Button::widestDigit;

int32_t Ted_Button::getWidestValue(int32_t minValue, int32_t maxValue, char S[12],
                                   uint16_t& w, uint16_t& h, bool showPlus) {
  int16_t dX, dY, dXcF;

  // Get width of each digit if we don't already have it.
  if (!haveDigitWidths) {
    char t[2];
    t[1] = 0;
    for (uint8_t i = 0; i < 10; i++) {
      t[0] = (char)((uint8_t)'0' + i);
      getLabelBounds(t, dX, dY, w, h, dXcF);
      digitWidths[i] = w;
      //monitor_printf(" digitWidths[%d] = %d\n", i, w);
    }

    // Get the widest digit.
    widestDigit = 1;
    for (uint8_t i = 2; i <= 9; i++)
      if (digitWidths[i] > digitWidths[widestDigit])
        widestDigit = i;

    haveDigitWidths = true;
  }

  // There are two halves here, negative values and positive values. One or the
  // other might be absent. If neither is absent, in order not to have to deal
  // with both in the code below, we'll call recursively twice, computing first
  // the negative half and then the positive half.
  if (minValue < 0 && maxValue >= 0) {
    int32_t V = getWidestValue(minValue, -1, S, w, h, showPlus);
    uint16_t W, H;
    char S2[12];
    int32_t Vpos = getWidestValue(0, maxValue, S2, W, H, showPlus);
    if (W > w) {
      V = Vpos;
      w = W;
      h = H;
      strcpy(S, S2);
    }
    return(V);
  }

  // We are doing just one side here, either negative or positive. They are
  // done the same way except the sign. Put it in signChar and make both
  // minValue and maxValue > 0. Then we only need do it one way.
  char signChar = showPlus ? '+' : ' ';
  if (minValue < 0) {
    signChar = '-';
    int32_t tmp = maxValue;
    maxValue = -minValue;
    minValue = -tmp;
  }

  // minValue >= 0, maxValue >= minValue.
  //monitor_printf(" minValue: %ld  maxValue: %ld  signChar: %c\n", minValue, maxValue, signChar);

  // We only need to compute widths of values that have the same number of
  // digits as maxValue. The easiest way to proceed is to change all digits
  // of maxValue after the leading digit to whatever digit is WIDEST, and
  // change the first digit to the widest one that is <= actual maxValue first
  // digit. Compute the width of that single value. It will be at least as wide
  // as the widest value string in the range, and possible wider.
  // Say maxValue is 101 and widest digit is 3. We would compute the width of
  // 133 and use that. If maxValue is 201, since digit 2 is wider than 1, we
  // would compute the width of 233.

  // Count digits in maxValue and get its first digit.
  uint32_t maxV = (uint32_t) maxValue;
  uint8_t Ndigits = 1;
  uint32_t pow10val = 1;
  uint32_t tmp2 = 10;
  while (tmp2 <= maxV) {
    Ndigits++;
    pow10val = tmp2;
    tmp2 *= 10;
  }
  uint8_t firstDigit = (uint8_t) (maxV / pow10val);
  //monitor_printf(" Ndigits: %d  pow10val: %ld  firstDigit: %d\n", Ndigits, pow10val, firstDigit);

  // Get the widest digit between 1 and firstDigit.
  uint8_t widestFirstDigit = 1;
  for (uint8_t i = 2; i <= firstDigit; i++)
    if (digitWidths[i] > digitWidths[widestFirstDigit])
      widestFirstDigit = i;
  //monitor_printf(" widestFirstDigit: %d\n", widestFirstDigit);

  // Create the test value by starting with widestFirstDigit and then inserting
  // widestDigit for each following position up to number of digits in maxValue.
  uint32_t testVal = widestFirstDigit;
  for (uint8_t i = 2; i <= Ndigits; i++)
    testVal = testVal * 10 + widestDigit;
  if (signChar == '-')
    testVal = -testVal;
  //monitor_printf(" testVal: %ld\n", testVal);

  // Convert the value to character and add the sign.
  char* p = S;
  if (signChar == '+')
    *p++ = signChar;
  sprintf(p, "%ld", testVal);
  //monitor_printf(" S: %s\n", S);

  // Compute the width and height.
  getLabelBounds(S, dX, dY, w, h, dXcF);
  //monitor_printf(" w: %d  h: %d\n", w, h);

  return(testVal);
}

/**************************************************************************/

void Ted_Button::getDegreeSymSize(int8_t& dx, int8_t& dy, int8_t& xa,
    uint8_t& d, uint8_t& rO, uint8_t& rI) {

  // The degree symbol has turned into a royal pain in the butt.
  // Obviously, ideally we would just have a symbol in the font that was in fact
  // a degree symbol. We don't have that and I don't think it wise to add one
  // myself. But the alternative is complicated. It is made complicated by the
  // way the fonts work. First, there is the cursor position for drawing a
  // character, which is sort of lower-left, but by "lower" we mean the baseline
  // without including characters with descenders. Then, each character is
  // defined with a (dx,dy) offset of its upper-left corner (of the bounding
  // box around the character) from the cursor position, and the width and
  // height of that box. The left side of the box can actually be to the LEFT
  // OR RIGHT of the cursor x-position. The box might lie entirely ABOVE the
  // baseline (e.g. the " character). These numbers figure into computing the
  // full width and height of a text string AND computing the place to position
  // the cursor for printing the string, given the upper-left corner position
  // of the printed string. It doesn't work well to just make the degree symbol
  // a fixed size that lies against the top of the text bounding box and against
  // its left side.
  // Here is how we will handle this: we'll make the degree symbol's bounding
  // box upper-left corner, relative to the cursor position, be EXACTLY THE SAME
  // AS THAT OF THE " character, which we can find from the font. And, we'll
  // make the DIAMETER of the degree symbol the same as the HEIGHT of the "
  // character.
  // The returned dx and dy values are the amount to ADD to the initial cursor
  // position to get the position of the upper-left corner of the degree symbol
  // bounding box, and xa is the amount to advance the cursor y-position from
  // where the degree symbol is printed to get the y-position of the next char.

  // Get xOffset, yOffset, xAdvance, and height of character '"' using the current
  // font table.
  uint8_t first = pgm_read_byte(&_f->first);
  GFXglyph* glyph;
  glyph = pgm_read_glyph_ptr(_f, '"' - first);
  dx = pgm_read_byte(&glyph->xOffset);
  dy = pgm_read_byte(&glyph->yOffset);
  xa = pgm_read_byte(&glyph->xAdvance);
  d = pgm_read_byte(&glyph->height);

  // Compute radius of outside and inside of degree symbol. Make the difference
  // between the two be about 3/11 of the diameter.
  rO = d/2; // Round down.
  if (rO < 4) {
    rO = 4;
    if (d < 2*rO) {
      xa += 2*rO - d;
      d = 2*rO;
    }
  }
  uint8_t dr = 3*d/11; // Round down.
  if (dr > rO)
    dr = rO;
  rI = rO - dr;
  if (rI < 2)
    rI = 2;
}

/**************************************************************************/

void Ted_Button::updateLabelSizeForDegreeSymbol(int16_t dX,
  int16_t& dY, int16_t& dXcF, uint16_t& w, uint16_t& h) {

  // Initially, the arguments reflect the size and relative cursor position of
  // the label without the degree symbol. We update them to include the degree
  // symbol.

  // Update dY, the offset from cursor y (baseline y coord) to top of label
  // bound box.
  dY = min(dY, _dy_degree);

  // Update w, the total width of the label. See comments in getLabelBounds().
  // This uses dXcF before updating below.
  w = dXcF - dX + _dx_degree + _d_degree;

  // Update h, the total height of the label. The height is the maximum label
  // y-coord minus the minimum y-coord. The min and max delta coords for the
  // label without the degree symbol are dY and dY+h. The min and max delta
  // coords of the degree symbol are _dy_degree and _dy_degree+_d_degree.
  h = max(dY+h, _dy_degree+_d_degree) - min(dY, _dy_degree);

  // Update dXcF, the cursor x-position after the end of the label and degree
  // symbol.
  dXcF += _xa_degree;
}

/**************************************************************************/

void Ted_Button::initButton(Adafruit_GFX *gfx, const char* align,
                int16_t x, int16_t y, int16_t w, int16_t h,
                uint16_t outlineColor, uint16_t fillColor, uint16_t textColor,
                const char* textAlign, char *label, bool degreeSym,
                uint8_t textSize_x, uint8_t textSize_y,
                const GFXfont* f, int16_t rCorner,
                uint8_t expU, uint8_t expD, uint8_t expL, uint8_t expR) {
  _textcolor = textColor;
  _textalign = "CC";
  setTextAlign(textAlign);
  _textsize_x = textSize_x;
  _textsize_y = textSize_y;
  _label = NULL;
  setLabel(label);
  _degreeSym = degreeSym;
  _f = f;
  _rCorner = rCorner;
  _w_label = _h_label = 0;
  _dx_degree = _dy_degree = _xa_degree = _d_degree = _rO_degree = _rI_degree = 0;

  Ted_Button_Base::initButton(gfx); // Pre-initialize base class with null values except gfx.

  if (gfx == 0)
    return;

  // Get label width/height, delta x/y from cursor to top-left, and final cursor x-coord.
  _gfx->setTextSize(_textsize_x, _textsize_y);
  _gfx->setFont(_f);
  int16_t dX, dY, dXcF;
  getLabelBounds(_label, dX, dY, _w_label, _h_label, dXcF);
  //monitor_printf("name: %s  label: %s  degreeSym: %d  w_label: %d  h_label: %d  dX: %d  dY: %d  dXcF: %d\n",
  // _name, _label, _degreeSym, _w_label, _h_label, dX, dY, dXcF);

  // If degree symbol is used, get its size data and use it to update dY, dXcF,
  // _w_label, and _h_label.
  if (_degreeSym) {
    getDegreeSymSize(_dx_degree, _dy_degree, _xa_degree, _d_degree, _rO_degree, _rI_degree);
    //monitor_printf("  Degree symbol d: %d  rO: %d  rI: %d  dX: %d  dY: %d  xA: %d\n",
    // _d_degree, _rO_degree, _rI_degree, _dx_degree, _dy_degree, _xa_degree);

    // Update the button size and delta cursor parameters for the effect of the
    // button.
    updateLabelSizeForDegreeSymbol(dX, dY, dXcF, _w_label, _h_label);

    //monitor_printf("  Final w_label: %d  h_label: %d  dY_label: %d  dXcF_label: %d\n",
    // _w_label, _h_label, dY, dXcF);
  }

  // Compute the actual button width/height in the case where w and/or h is
  // non-positive, by adding to _w_label/_h_label the absolute value of w/h
  // and twice the corner radius. We need to make sure the label does not run
  // into the curved corner, by placing those corners outside the label bounding
  // rectangle.
  if (w <= 0)
    w = _w_label - w + _rCorner*2;
  if (h <= 0)
    h = _h_label - h + _rCorner*2;
  //monitor_printf("  Final w: %d  h: %d\n", w, h);

  // Compute the upper-left coords of the entire button rectangle, (xL, yT),
  // using _align, x, y, w and h.
  if (align[0] == 'C' && align[1] == 0)
    align = "CC";

  int16_t xL = x;
  if (align[1] == 'R')
    xL += 1 - w;
  else if (align[1] == 'C')
    xL += 1 - w/2;

  int16_t yT = y;
  if (align[0] == 'B')
    yT += 1 - h;
  else if (align[0] == 'C')
    yT += 1 - h/2;
  //monitor_printf("  xL: %d  yT: %d\n", xL, yT);

  // Now we have the info we need to call the base class initButton().
  Ted_Button_Base::initButton(gfx, xL, yT, (uint16_t) w, (uint16_t) h,
    outlineColor, fillColor, expU, expD, expL, expR);
}

/**************************************************************************/

bool Ted_Button::setTextColor(uint16_t textColor) {
  if (_textcolor != textColor) {
    _textcolor = textColor;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button::setTextAlign(const char* textAlign) {
  if (textAlign[0] == 'C' && textAlign[1] == 0)
    textAlign = "CC";
  if (strcmp(_textalign, textAlign) != 0) {
    _textalign = textAlign;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button::setTextSize(uint8_t textSize_x, uint8_t textSize_y) {
  if (_textsize_x != textSize_x || _textsize_y != textSize_y) {
    _textsize_x = textSize_x;
    _textsize_y = textSize_y;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button::setFont(const GFXfont* f) {
  if (_f != f) {
    _f = f;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button::setLabel(const char* label) {
  if (_label != NULL) {
    if (strcmp(label, _label) == 0)
      return(false);
    if (strlen(label) != strlen(_label)) {
      free(_label);
      _label = NULL;
    }
  }
  if (_label == NULL)
    _label = (char*) malloc(strlen(label)+1);
  strcpy(_label, label);
  _changedSinceLastDrawn = true;
  return(true);
}

/**************************************************************************/

void Ted_Button::drawButton(bool inverted) {
  _inverted = inverted;

  uint16_t fill, outline, text;
  if (!_inverted) {
    fill = _fillcolor;
    outline = _outlinecolor;
    text = _textcolor;
  } else {
    fill = _textcolor;
    outline = _outlinecolor;
    text = _fillcolor;
  }

  if (fill != TRANSPARENT_COLOR || outline != TRANSPARENT_COLOR) {
    if (_rCorner == 0) {
      if (fill != TRANSPARENT_COLOR)
        _gfx->fillRect(_xL, _yT, _w, _h, fill);
      if (outline != TRANSPARENT_COLOR)
        _gfx->drawRect(_xL, _yT, _w, _h, outline);
    } else {
      if (fill != TRANSPARENT_COLOR)
        _gfx->fillRoundRect(_xL, _yT, _w, _h, _rCorner, fill);
      if (outline != TRANSPARENT_COLOR)
        _gfx->drawRoundRect(_xL, _yT, _w, _h, _rCorner, outline);
    }
  }

  if (_label[0] != 0 && text != TRANSPARENT_COLOR) {
    // We must recompute the size of the label, as it may be a different label
    // now than the one used in initButton(), and we want to align the label
    // according to _textalign.
    _gfx->setTextSize(_textsize_x, _textsize_y);
    _gfx->setFont(_f);
    int16_t dX, dY, dXcF;
    uint16_t W, H;
    getLabelBounds(_label, dX, dY, W, H, dXcF);
    if (_degreeSym) {
      updateLabelSizeForDegreeSymbol(dX, dY, dXcF, W, H);
    }

    // Compute the initial cursor position to use, which may be ABOVE and LEFT
    // OR RIGHT of the lower-left corner. This is because print() uses the
    // cursor y-position as the location of the BASELINE of the text when it
    // writes the text to the display. If the text has characters with
    // descenders, they will descend BELOW that line. Also, each character
    // may start slightly to the left or right of the cursor x-position.
    // The original code did not account for this and assumed that the lower
    // left corner of the text label bounding box was the correct position
    // for the cursor for drawing the text.
    // The label is aligned within the button according to _textalign.
    // (xStart, yBase) is the coords to set cursor to print the label

    int16_t xStart = _xL - dX;
    if (_textalign[1] == 'R')
      xStart += _w - W;
    else if (_textalign[1] == 'C')
      xStart += _w/2 - W/2;

    int16_t yBase = _yT - dY;
    if (_textalign[0] == 'T')
      yBase += _h - H;
    else if (_textalign[0] == 'C')
      yBase += _h/2 - H/2;

    //monitor_printf("Name: %s  Label: %s  xStart: %d  yBase: %d\n", _name, _label, xStart, yBase);
    // Now display the label.
    _gfx->setCursor(xStart, yBase);
    _gfx->setTextColor(text);
    _gfx->print(_label);

    // Also print degree symbol if enabled.
    if (_degreeSym) {
      // Compute coords of center of circle.
      int16_t X = _gfx->getCursorX() + _dx_degree + _rO_degree;
      int16_t Y = _gfx->getCursorY() + _dy_degree + _rO_degree;
      _gfx->fillCircle(X, Y, _rO_degree, text);
      _gfx->fillCircle(X, Y, _rI_degree, fill);
    }
  }

  _changedSinceLastDrawn = false;
}

/**************************************************************************/

bool Ted_Button::setLabelAndDrawIfChanged(const char* label, bool forceDraw) {
  setLabel(label);
  if (_changedSinceLastDrawn || forceDraw) {
    drawButton();
    return(true);
  }
  return(false);
}

// -------------------------------------------------------------------------
