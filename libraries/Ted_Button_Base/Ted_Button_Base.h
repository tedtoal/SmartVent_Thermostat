#ifndef _TED_BUTTON_BASE_H
#define _TED_BUTTON_BASE_H
/*
  This code is a modification of class Adafruit_GFX_Button in the library
  Adafruit_GFX_Library.  That library didn't center the text properly in
  the button when the text is made using a custom font. That class has been
  split into two classes, this base class (from which other types of button
  classes can be derived) and class Ted_Button, which fixes the centering
  problem and adds other features.
*/

#include <Adafruit_GFX.h>

// Use this color to avoid having outline, button background, or label drawn.
// Note: The ILI9341 controller actually uses all 16 bits as color info, 5 bits
// for red and blue, and 6 bits for green. It internally maps these to 6 bits
// for each color. Therefore, every 16-bit combination is a valid color. We
// would like to use an invalid value to represent a transparent color, but
// there is no invalid value. So, what we will do is arbitrarily pick a color
// value that is unlikely to be used anywhere. We will choose with the least
// significant bit of the R, G, and B values being 1 and all other bits 0.
const uint16_t TRANSPARENT_COLOR = 0x0841;

class Ted_Button_Base {

protected:
  const char* _name;
  Adafruit_GFX* _gfx;
  int16_t _xL, _yT; // Coordinates of top-left corner
  uint16_t _w, _h;
  uint16_t _expU, _expD, _expL, _expR;
  uint16_t _outlinecolor, _fillcolor;
  int16_t _delta;
  bool _inverted;
  bool _changedSinceLastDrawn; // Set TRUE if any visible attribute changes, cleared when button drawn
  bool _isPressed;
  bool _returnedLastAction;

public:
  /**************************************************************************/
  /*!
   @brief    Constructor.
   @param    name         String giving a name to the button, for debugging purposes only!
   @param    (others)     Remaining (optional) arguments are the same as initButton() below
  */
  /**************************************************************************/
  Ted_Button_Base(const char* name, Adafruit_GFX* gfx=0,
                  int16_t xL=0, int16_t yT=0, uint16_t w=0, uint16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0) :
            _name(name) {
    initButton(gfx, xL, yT, w, h, outlineColor, fillColor, expU, expD, expL, expR);
  }


  /**************************************************************************/
  /*!
   @brief    Initialize button
   @param    gfx     Pointer to our display so we can draw to it!
   @param    xL      The X coordinate of the top-left corner of the button
   @param    yT      The Y coordinate of the top-left corner of the button
   @param    w       Width of the button in pixels
   @param    h       Height of the button in pixels
   @param    outlineColor Color of the outline (16-bit 5-6-5 standard)
   @param    fillColor    Color of the button fill (16-bit 5-6-5 standard)
   @param    expU    Expand button up by this when contains() tests a point
   @param    expD    Expand button down by this when contains() tests a point
   @param    expL    Expand button left by this when contains() tests a point
   @param    expR    Expand button right by this when contains() tests a point
  */
  /**************************************************************************/
  void initButton(Adafruit_GFX* gfx=0,
                  int16_t xL=0, int16_t yT=0, uint16_t w=0, uint16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0);

  /**************************************************************************/
  /*!
   @brief    Get current outline color for button
   @returns  The current outline color
  */
  /**************************************************************************/
  uint16_t getOutlineColor(void) { return(_outlinecolor); }

  /**************************************************************************/
  /*!
   @brief    Set new outline color for button
   @param    outlineColor   The new outline color
   @returns  true if new color is different from old
  */
  /**************************************************************************/
  bool setOutlineColor(uint16_t outlineColor);

  /**************************************************************************/
  /*!
   @brief    Get current fill color for button
   @returns  The current fill color
  */
  /**************************************************************************/
  uint16_t getFillColor(void) { return(_fillcolor); }

  /**************************************************************************/
  /*!
   @brief    Set new fill color for button
   @param    fillColor   The new fill color
   @returns  true if new color is different from old
  */
  /**************************************************************************/
  bool setFillColor(uint16_t fillColor);

  /**************************************************************************/
  /*!
   @brief    Get inversion flag for last draw
   @returns  The last value of "inverted" used to draw the button
  */
  /**************************************************************************/
  uint16_t getInverted(void) { return(_inverted); }

  /**************************************************************************/
  /*!
   @brief    Draw the button on the screen
   @param    inverted   Whether to draw with fill/text swapped to indicate
                        'pressed'
  */
  /**************************************************************************/
  virtual void drawButton(bool inverted);

  /**************************************************************************/
  /*!
   @brief    Draw the button on the screen, using value of "inverted" from last
              call to drawButton()
  */
  /**************************************************************************/
  virtual void drawButton() { drawButton(_inverted); }

  /**************************************************************************/
  /*!
   @brief    If any button attribute has changed since the button was last
              drawn, redraw the button.
   @param    forceDraw  If true, the button is drawn even if attributes have
              not changed.
   @returns  true if button was drawn
  */
  /**************************************************************************/
  virtual bool drawIfChanged(bool forceDraw = false);

  /**********************************************************************/
  /*!
    @brief    Sets button to the pressed state and draws it inverted
  */
  /**********************************************************************/
  void press() {
    if (!_isPressed) {
      _isPressed = true;
      _returnedLastAction = false;
      drawButton(_isPressed);
    }
  }

  /**********************************************************************/
  /*!
    @brief    Sets button to the released state and draws it non-inverted
  */
  /**********************************************************************/
  void release() {
    if (_isPressed) {
      _isPressed = false;
      _returnedLastAction = false;
      drawButton(_isPressed);
    }
  }

  /**********************************************************************/
  /*!
    @brief    Query whether the button is currently pressed
    @returns  true if pressed
  */
  /**********************************************************************/
  bool isPressed(void) { return _isPressed; };

  /**************************************************************************/
  /*!
   @brief    Query whether the button was pressed since we last checked state
   @returns  true if was not-pressed before, now is.
  */
  /**************************************************************************/
  bool justPressed() {
    if (!_isPressed || _returnedLastAction) return(false);
    _returnedLastAction = true;
    return(true);
  }

  /**************************************************************************/
  /*!
   @brief    Query whether the button was released since we last checked state
   @returns  true if was pressed before, now is not.
  */
  /**************************************************************************/
  bool justReleased() {
    if (_isPressed || _returnedLastAction) return(false);
    _returnedLastAction = true;
    return(true);
  }

  /**************************************************************************/
  /*!
    @brief    Test if a coordinate is within the bounds of the button
    @param    x       The X coordinate to check
    @param    y       The Y coordinate to check
    @returns  true if within button graphics outline
  */
  /**************************************************************************/
  bool contains(int16_t x, int16_t y);

  /**************************************************************************/
  /*!
    @brief    Return a value that is the amount by which to change some other
              value, used for derived classes that act as "increment" or
              "decrement" buttons
    @returns  Value by which to change another value, 0 if not implemented in
              this class because the class is not an increment/decrement class
  */
  /**************************************************************************/
  virtual int16_t delta(void) { return(_delta); }

};

#endif // _TED_BUTTON_BASE_H
