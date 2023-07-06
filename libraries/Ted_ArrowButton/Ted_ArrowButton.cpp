#include "Ted_ArrowButton.h"

/**************************************************************************/

void Ted_ArrowButton::initButton(Adafruit_GFX *gfx,
                char orient, const char* align,
                int16_t x, int16_t y, uint16_t s1, uint16_t s2,
                uint16_t outlineColor, uint16_t fillColor,
                uint8_t expU, uint8_t expD, uint8_t expL, uint8_t expR) {
  _orient = orient;
  _s1 = s1;
  _s2 = s2;

  Ted_Button_Base::initButton(gfx); // Pre-initialize base class with null values except gfx.

  if (gfx == 0)
    return;

  if (align[0] == 'C' && align[1] == 0) align = "CC";

  // Use orientation and b and d to determine bounding box size (w, h).
  uint16_t w, h;
  // If s1 == s2 then they are equilateral triangles and its easy, but that
  // does not have to be the case. If U or D, the other two sides of length s2
  // are the hypotenuses of right triangles whose base is s1/2, giving the
  // height of the triangle as sqrt(s2^2 - s1^2/4).  If L or R, reverse the
  // role of s1 and s2.
  w = s1;
  h = (uint16_t)(1.0+sqrt(s2*s2 - s1*s1/4));
  if (orient == 'L' || orient == 'R') {
    w = h;
    h = s1;
  }

  // Compute xL and yT using align, x, y, w and h.
  int16_t xL, yT;
  if (align[1] == 'L') xL = x;
  else if (align[1] == 'R') xL = x - w + 1;
  else xL = x - w/2 + 1;

  if (align[0] == 'T') yT = y;
  else if (align[0] == 'B') yT = y - h + 1;
  else yT = y - h/2 + 1;

  // Compute coords of vertices using orient, xL, yT, w, and h.
  _x0 = _x1 = _x2 = xL;
  _y0 = _y1 = _y2 = yT;
  if (orient == 'U') {
    _x0 += w/2;
    _x1 += w;
    _y1 += h;
    _y2 += h;
  } else if (orient == 'D') {
    _x0 += w/2;
    _y0 += h;
    _x2 += w;
  } else if (orient == 'L') {
    _y0 += h/2;
    _x1 += w;
    _x2 += w;
    _y2 += h;
  } else { // Assume 'R'
    _x0 += w;
    _y0 += h/2;
    _y1 += h;
  }

  Ted_Button_Base::initButton(gfx, xL, yT, w, h, outlineColor, fillColor,
    expU, expD, expL, expR);

  _delta = (orient == 'L' || orient == 'U') ? -1 : +1;
}

/**************************************************************************/

void Ted_ArrowButton::drawButton(bool inverted) {
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
    _gfx->fillTriangle(_x0, _y0, _x1, _y1, _x2, _y2, fill);
  if (outline != TRANSPARENT_COLOR)
    _gfx->drawTriangle(_x0, _y0, _x1, _y1, _x2, _y2, outline);

  _changedSinceLastDrawn = false;
}

// -------------------------------------------------------------------------
