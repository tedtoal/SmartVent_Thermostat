#ifndef _TED_BUTTON_H
#define _TED_BUTTON_H
/*
  This code is a modification of class Adafruit_GFX_Button in the library
  Adafruit_GFX_Library.  That library didn't center the text properly in
  the button when the text is made using a custom font. The Adafruit code
  is now partly in the base class Ted_Button_Base and partly in its derived
  class defined here, Ted_Button.
  This class and the base class have these changes over the Adafruit class:
    - text centered properly
    - functions added for get/set outline, fill, text color, text size, label
    - added setLabelAndDrawIfChanged() function
    - constructors changed to a single one with a new first argument "align"
    - custom font can be set for the button
    - special TRANSPARENT_COLOR added to allow a color that causes no element
      to be drawn, the background color remains, applied to outline, fill, label
    - w and h (width and height of button) changed from unsigned to signed,
      and if they are specified non-positive AND IF gfx is not 0, then to get
      actual button width and height, LABEL WIDTH/HEIGHT is ADDED TO ABSOLUTE w/h
    - made initButton() compute width and height and lower-left coords of the
        button label and store it in class variables for use by drawButton()
        etc. Corrected the code for use of the text baseline in print().
    - added drawDegreeSymbol() function and degreeSym argument to init()
    - maximum button label length changed from 9 to 20.
*/

#include <Ted_Button_Base.h>

class Ted_Button : public Ted_Button_Base {

protected:
  uint8_t _textsize_x;
  uint8_t _textsize_y;
  uint16_t _textcolor;
  const char* _textalign;
  const GFXfont* _f;
  char* _label;
  bool _degreeSym;
  int16_t _rCorner;
  uint16_t _w_label, _h_label;
  // Degree symbol data:
  //  _dx_degree: distance from degree initial cursor to left of degree bound box
  //  _dy_degree: distance from degree initial cursor to top of degree bound box
  //  _xa_degree: amount to advance y-coordinate of cursor for degree symbol
  //  _d_degree: diameter of degree symbol (its width AND height)
  //  _rO_degree: outer radius of degree symbol
  //  _rI_degree: inner radius of degree symbol
  int8_t _dx_degree, _dy_degree, _xa_degree;
  uint8_t _d_degree, _rO_degree, _rI_degree;

  // Used by getWidestValue().
  static uint16_t digitWidths[10];
  static bool haveDigitWidths;
  static uint8_t widestDigit;

  /**************************************************************************/
  // Compute pixel size of label string 'str' using current font/size. Other
  // arguments are references to variables in which to return label size and
  // positioning information:
  //  dX: Delta x coordinate from left side  of label to first character cursor
  //    x-position. Subtract from left-side x-coordinate to get cursor x-coord.
  //  dY: Delta Y coordinate from top side of label to first character cursor
  //    y-position. Subtract from top-side y-coordinate to get cursor y-coord.
  //  w:  Text bounding rectangle width
  //  h:  Text bounding rectangle height
  //  dXcF: Delta x from starting to ending cursor X coordinate.
  // The correct cursor position to use when printing the text, if the text
  // upper-left corner is to be at (X1,Y1), is:
  //    (X1-dX, Y1-dY)
  // Note that dX might be 0 or a small negative or positive value, and dY will
  // almost always be a negative value that is the distance from the text
  // baseline to dY. Note in particular that the text baseline is NOT Y1+h. If
  // the cursor position is set to (X1, Y1+h) and the text printed, it will be
  // shifted left or right slightly when the first character starts slightly
  // left or right of the cursor position (which some characters do), and the
  // text will be shifted up if it contains any characters with descenders that
  // extend below the baseline. The final cursor position is (X1-dX+dXcF, Y1-dY),
  // which is NOT the same as (dX+w, dY+h). The cursor position to use in order
  // to print additional text after the label, if the upper-left corner of the
  // text is at position (X1, Y1), is:
  //  (X1-dX+dXcF, Y1-dY)
  // The position of the lower-right corner of the text bounding box, (X2, Y2),
  // is: (X1+w, Y1+h)
  // If another character is printed after the label, using the starting cursor
  // position (X1-dX+dXcF, Y1-dY), and that new character has top-left bound box
  // offset (DX, DY) and width W and height H, then the new lower-right corner
  // of the text bounding box, (X2, Y2), is:
  //  (X1-dX+dXcF+DX+W, Y1-dY+max(0, DY+H))
  // and so the new total text string width and height are:
  //  new w = dXcF-dX+DX+W = new X2 - X1
  //  new h = -dY+max(0, DY+H) = new Y2 - Y1
  /**************************************************************************/
  void getLabelBounds(const char *str, int16_t& dX, int16_t& dY,
                      uint16_t& w, uint16_t& h, int16_t& dXcF);

  /**************************************************************************/
  // Given a range of integer values, determine the value in that range that,
  // when converted to a string, has the widest pixel width using the current
  // font/size. Return that widest value, return its string in S, and return
  // the string width and height in pixels in w and h. If showPlus is true,
  // positive values have a leading "+" sign in the character string.
  // Note: In order to keep the code simple, a shortcut is taken that results
  // in the possibility that the returned value (and its string in S) might not
  // be within the given range. It will be an approximation of the widest value,
  // but its width may be wider than the actual widest value. For example, if
  // minValue were -10 and maxValue were +120 and showPlus were true, the
  // returned value might be 133 and returned string "+133".
  /**************************************************************************/
  int32_t getWidestValue(int32_t minValue, int32_t maxValue, char S[12],
                         uint16_t& w, uint16_t& h, bool showPlus=false);

  /**************************************************************************/
  // Compute degree symbol delta x/y from cursor position to upper-left corner,
  // cursor x-position advance amount, diameter, and outer and inner radius.
  /**************************************************************************/
  void getDegreeSymSize(int8_t& dx, int8_t& dy, int8_t& xa, uint8_t& d,
    uint8_t& rO, uint8_t& rI);

  /**************************************************************************/
  // Update the arguments that are references by adding in the effect of putting
  // the degree symbol after the label. On call, the arguments reflect the size
  // and relative cursor position of the label without the degree symbol. This
  // requires that _dx_degree, _dy_degree, _xa_degree, and _d_degree have been
  // set correctly for the degree symbol.
  /**************************************************************************/
  void updateLabelSizeForDegreeSymbol(int16_t dX,
    int16_t& dY, int16_t& dXcF, uint16_t& w, uint16_t& h);

public:
  /**************************************************************************/
  /*!
   @brief    Constructor.
   @param    name         String giving a name to the button, for debugging purposes only!
   @param    (others)     Remaining (optional) arguments are the same as initButton() below
  */
  /**************************************************************************/
  Ted_Button(const char* name, Adafruit_GFX* gfx=0, const char* align="C",
                  int16_t x=0, int16_t y=0, int16_t w=0, int16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint16_t textColor=0, const char* textAlign="C", char* label=0,
                  bool degreeSym=false, uint8_t textSize_x=1, uint8_t textSize_y=1,
                  const GFXfont* f=NULL, int16_t rCorner=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0) :
            Ted_Button_Base(name) {
    initButton(gfx, align, x, y, w, h, outlineColor, fillColor, textColor,
      textAlign, label, degreeSym, textSize_x, textSize_y, f, rCorner,
      expU, expD, expL, expR);
  }

  /**************************************************************************/
  /*!
   @brief    Destructor. Release memory used by _label.
  */
  /**************************************************************************/
  ~Ted_Button() {
    if (_label != NULL) {
      free(_label);
      _label = NULL;
    }
  }

  /**************************************************************************/
  /*!
   @brief    Initialize button with our desired color/size/etc. settings
   @param    gfx     See Ted_Button_Base::initButton()
   @param    align   (x,y) alignment: TL, TC, TR, CL, CC, CR, BL, BC, BR, C
                     where T=top, B=bottom, L=left, R=right, C=center, C=CC
   @param    x       The X coordinate of the button, relative to 'align'
   @param    y       The X coordinate of the button, relative to 'align'
   @param    w       Width of the button, non-positive to compute width from label and add abs(w)
   @param    h       Height of the button, non-positive to compute height from label and add abs(h)
   @param    outlineColor See Ted_Button_Base::initButton()
   @param    fillColor    See Ted_Button_Base::initButton()
   @param    textColor    Color of the button label (16-bit 5-6-5 standard)
   @param    textAlign    Like 'align' but gives alignment for label, 1st char = up/down alignment, 2nd = left/right
   @param    label        String of the text inside the button, should be the largest
                          possible label if w or h is negative to ensure all possible
                          labels will fit in the size that is automatically computed
   @param    degreeSym    If true, a degree symbol is drawn at the end of the label
   @param    textSize_x   Font magnification in X-axis of the label text
   @param    textSize_y   Font magnification in Y-axis of the label text
   @param    f       Custom font to use for the label, NULL for default font
   @param    rCorner Radius of rounded corners of button outline rectangle, 0 for none
   @param    expU    See Ted_Button_Base::initButton()
   @param    expD    See Ted_Button_Base::initButton()
   @param    expL    See Ted_Button_Base::initButton()
   @param    expR    See Ted_Button_Base::initButton()
  */
  /**************************************************************************/
  void initButton(Adafruit_GFX* gfx=0, const char* align="C",
                  int16_t x=0, int16_t y=0, int16_t w=0, int16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0, uint16_t textColor=0,
                  const char* textAlign="C", char* label=0, bool degreeSym=false,
                  uint8_t textSize_x=1, uint8_t textSize_y=1,
                  const GFXfont* f=NULL, int16_t rCorner=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0);

  /**************************************************************************/
  /*!
   @brief    Get current text color for button label
   @returns  The current text color
  */
  /**************************************************************************/
  uint16_t getTextColor(void) { return(_textcolor); }

  /**************************************************************************/
  /*!
   @brief    Set new color for button label
   @param    textColor   The new text color
   @returns  true if new color is different from old
  */
  /**************************************************************************/
  bool setTextColor(uint16_t textColor);

  /**************************************************************************/
  /*!
   @brief    Get current alignment for button label
   @returns  The label alignment setting
  */
  /**************************************************************************/
  const char* getTextAlign(void) { return(_textalign); }

  /**************************************************************************/
  /*!
   @brief    Set new alignment for button label
   @param    textAlign   The new text alignment
   @returns  true if new alignment is different from old
  */
  /**************************************************************************/
  bool setTextAlign(const char* textAlign);

  /**************************************************************************/
  /*!
   @brief    Get current text size for button label
   @param    textSize_x  The current x-direction text size is returned here
   @param    textSize_y  The current y-direction text size is returned here
  */
  /**************************************************************************/
  void getTextSize(uint8_t& textSize_x, uint8_t& textSize_y) {
    textSize_x = _textsize_x; textSize_y = _textsize_y;
  }

  /**************************************************************************/
  /*!
   @brief    Set new text size for button label
   @param    textSize_x   The new x-direction text size
   @param    textSize_y   The new y-direction text size
   @returns  true if new size is different from old
  */
  /**************************************************************************/
  bool setTextSize(uint8_t textSize_x, uint8_t textSize_y);

  /**************************************************************************/
  /*!
   @brief    Get current button label font
   @returns  The current label font
  */
  /**************************************************************************/
  const GFXfont* getFont(void) { return(_f); }

  /**************************************************************************/
  /*!
   @brief    Set new font for button label
   @param    f   The new font
   @returns  true if new font is different from old
  */
  /**************************************************************************/
  bool setFont(const GFXfont* f=NULL);

  /**************************************************************************/
  /*!
   @brief    Get current button label
   @returns  The current label text
  */
  /**************************************************************************/
  const char* getLabel(void) { return(_label); }

  /**************************************************************************/
  /*!
   @brief    Set new label for button
   @param    label   The new label
   @returns  true if new label is different from old label
   @note     The new label is copied to a buffer allocated in memory for it,
             after freeing memory used by any previous label, but if the new
             label is exactly the same length as the old one, the existing
             memory is simply reused
  */
  /**************************************************************************/
  bool setLabel(const char* label);

  /**************************************************************************/
  /*!
   @brief    Get flag indicating whether a degree symbol is drawn after the label
   @returns  true if degree symbol is drawn, else false
  */
  /**************************************************************************/
  bool getDegreeSymbol(void) { return(_degreeSym); }

  /**************************************************************************/
  /*!
   @brief    Draw the button on the screen
   @param    inverted   Whether to draw with fill/text swapped to indicate
                        'pressed'
  */
  /**************************************************************************/
  using Ted_Button_Base::drawButton;
  virtual void drawButton(bool inverted) override;

  /**************************************************************************/
  /*!
   @brief    Set new label for button and draw the button if the label changed
              or if any visible button attribute changed since last drawn.
   @param    label      The new label
   @param    forceDraw  If true, the button is drawn even if attributes have
              not changed.
   @returns  true if button was drawn
  */
  /**************************************************************************/
  bool setLabelAndDrawIfChanged(const char* label, bool forceDraw = false);

};

#endif // _TED_BUTTON_H
