#ifndef _TED_ARROWBUTTON_H
#define _TED_ARROWBUTTON_H
/*
  Define a new button class derived from class Ted_Button_Base, for creating
  graphical user interface buttons in the shape of equilateral triangles, to be
  used as arrow buttons, typically to signify increase or decrease in a numeric
  value. The virtual delta() function returns -1 for left or up-pointing buttons
  and +1 for right or down-pointing buttons.
*/

#include <Ted_Button_Base.h>

class Ted_ArrowButton :  public Ted_Button_Base {

protected:
  char _orient;
  int16_t _x0, _y0, _x1, _y1, _x2, _y2; // Triangle vertices, 0=tip, CW for 1 and 2
  uint16_t _s1, _s2; // Lengths of triangle sides, one side is _s1, 2 are _s2.

public:
  /**************************************************************************/
  /*!
   @brief    Constructor.
   @param    name         String giving a name to the button, for debugging purposes only!
   @param    (others)     Remaining (optional) arguments are the same as initButton() below
  */
  /**************************************************************************/
  Ted_ArrowButton(const char* name, Adafruit_GFX* gfx=0,
                  char orient='U', const char* align="C",
                  int16_t x=0, int16_t y=0, uint16_t s1=0, uint16_t s2=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0) :
            Ted_Button_Base(name) {
    initButton(gfx, orient, align, x, y, s1, s2, outlineColor, fillColor,
      expU, expD, expL, expR);
  }

  /**************************************************************************/
  /*!
   @brief    Initialize button with our desired color/size/etc. settings
   @param    gfx     Pointer to our display so we can draw to it!
   @param    orient  Orientation of triangle: U=UP, D=DOWN, L=LEFT, R=RIGHT
   @param    align   See Ted_Button::initButton()
   @param    x       See Ted_Button::initButton()
   @param    y       See Ted_Button::initButton()
   @param    s1      Length of the triangle that is opposite vertex at end of arrow
   @param    s2      Length of other two triangle sides
   @param    outlineColor See Ted_Button_Base::initButton()
   @param    fillColor    See Ted_Button_Base::initButton()
   @param    expU    See Ted_Button_Base::initButton()
   @param    expD    See Ted_Button_Base::initButton()
   @param    expL    See Ted_Button_Base::initButton()
   @param    expR    See Ted_Button_Base::initButton()
  */
  /**************************************************************************/
  void initButton(Adafruit_GFX* gfx=0,
                  char orient='U', const char* align="C",
                  int16_t x=0, int16_t y=0, uint16_t s1=0, uint16_t s2=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0);

  /**************************************************************************/
  /*!
   @brief    Return the button's orientation setting (orient argument to init)
   @returns  Single character giving orientation, U, D, L, or R
  */
  /**************************************************************************/
  char getOrientation(void) { return(_orient); }

  /**************************************************************************/
  /*!
   @brief    Draw the button on the screen
   @param    inverted   Whether to draw with fill/outline swapped to indicate
                        'pressed'
  */
  /**************************************************************************/
  using Ted_Button_Base::drawButton;
  virtual void drawButton(bool inverted) override;

};

#endif // _TED_ARROWBUTTON_H
