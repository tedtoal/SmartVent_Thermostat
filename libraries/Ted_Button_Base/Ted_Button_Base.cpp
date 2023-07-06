/*
  This was created by edits to the Adafruit_GFX_Button class in the library
  Adafruit_GFX_Library.
*/

#include "Ted_Button_Base.h"

/**************************************************************************/

void Ted_Button_Base::initButton(Adafruit_GFX *gfx,
                int16_t xL, int16_t yT, uint16_t w, uint16_t h,
                uint16_t outlineColor, uint16_t fillColor,
                uint8_t expU, uint8_t expD, uint8_t expL, uint8_t expR) {
  _gfx = gfx;
  _xL = xL;
  _yT = yT;
  _w = w;
  _h = h;
  _expU = expU;
  _expD = expD;
  _expL = expL;
  _expR = expR;
  _outlinecolor = outlineColor;
  _fillcolor = fillColor;
  _inverted = false;
  _changedSinceLastDrawn = true;
  _isPressed = false;
  _returnedLastAction = true;
  _delta = 0;
}

/**************************************************************************/

bool Ted_Button_Base::setOutlineColor(uint16_t outlineColor) {
  if (_outlinecolor != outlineColor) {
    _outlinecolor = outlineColor;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button_Base::setFillColor(uint16_t fillColor) {
  if (_fillcolor != fillColor) {
    _fillcolor = fillColor;
    _changedSinceLastDrawn = true;
    return(true);
  }
  return(false);
}

/**************************************************************************/

void Ted_Button_Base::drawButton(bool inverted) {
  _inverted = inverted;

  uint16_t fill, outline;
  if (!_inverted) {
    fill = _fillcolor;
    outline = _outlinecolor;
  } else {
    fill = _outlinecolor;
    outline = _fillcolor;
  }

  if (fill != TRANSPARENT_COLOR)
    _gfx->fillRect(_xL, _yT, _w, _h, fill);
  if (outline != TRANSPARENT_COLOR)
    _gfx->drawRect(_xL, _yT, _w, _h, outline);

  _changedSinceLastDrawn = false;
}

/**************************************************************************/

bool Ted_Button_Base::drawIfChanged(bool forceDraw) {
  if (_changedSinceLastDrawn || forceDraw) {
    drawButton(_inverted);
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button_Base::contains(int16_t x, int16_t y) {
  return ((x >= _xL-_expL) && (x < (int16_t)(_xL+_w+_expR)) &&
          (y >= _yT-_expU) && (y < (int16_t)(_yT+_h+_expD)));
}

// -------------------------------------------------------------------------
