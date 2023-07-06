#include "Ted_Button_int8.h"

/**************************************************************************/

void Ted_Button_int8::initButton(Adafruit_GFX *gfx, const char* align,
                  int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t outlineColor, uint16_t fillColor,
                  uint16_t textColor, const char* textAlign,
                  uint8_t textSize_x, uint8_t textSize_y,
                  const GFXfont* f, int16_t rCorner,
                  int8_t value, int8_t minValue, int8_t maxValue,
                  bool degreeSym, bool showPlus,
                  int8_t (*checkValue)(Ted_Button_int8& btn, int8_t value),
                  uint8_t expU, uint8_t expD, uint8_t expL, uint8_t expR) {
  _value = value;
  _minValue = minValue;
  _maxValue = maxValue;
  _showPlus = showPlus;
  _checkValue = checkValue;

  Ted_Button::initButton(gfx); // Pre-initialize base class with null values except gfx.

  if (gfx == 0)
    return;

  // For the initial value of the label, find the longest value in the range
  // minValue..maxValue and use that. The label might actually be out of that
  // range a bit, but it doesn't matter. We set 'value' as the actual button
  // value (including its label) AFTER calling Ted_Button::initButton below.
  char widestValStr[12];
  uint16_t W, H;
  gfx->setTextSize(textSize_x, textSize_y);
  gfx->setFont(f);
  Ted_Button::getWidestValue(minValue, maxValue, widestValStr, W, H, showPlus);
  Ted_Button::initButton(gfx, align, x, y, w, h, outlineColor, fillColor, textColor,
                  textAlign, widestValStr, degreeSym, textSize_x, textSize_y, f, rCorner,
                  expU, expD, expL, expR);

  // Now set the correct value, applying limits.
  setValue(value);
}

/**************************************************************************/

bool Ted_Button_int8::setValue(int8_t value, bool dontCheck) {
  if (value < _minValue)
    value = _minValue;
  if (value > _maxValue)
    value = _maxValue;
  if (!dontCheck && _checkValue != NULL)
    value = _checkValue(*this, value);
  if (value == _value)
    return(false);
  _value = value;
  char S[5];
  char* p = S;
  if (_value > 0 && _showPlus)
    *p++ = '+';
  itoa(_value, p, 10);
  setLabel(S);
  _changedSinceLastDrawn = true;
  return(true);
}

/**************************************************************************/

bool Ted_Button_int8::setValueAndDrawIfChanged(int8_t value, bool forceDraw) {
  setValue(value);
  if (_changedSinceLastDrawn || forceDraw) {
    drawButton();
    return(true);
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button_int8::valueIncDec(int8_t N, Ted_Button_Base* btn) {
  if (btn != NULL && (int8_t)btn->delta() != 0)
    N = (int8_t)btn->delta();
  int8_t newValue;
  if (N >= 0) {
    newValue = (_value > _maxValue - N) ? _maxValue : _value + N;
  } else {
    N = -N;
    newValue = (_value < _minValue + N) ? _minValue : _value - N;
  }
  return(setValueAndDrawIfChanged(newValue));
}

// -------------------------------------------------------------------------
