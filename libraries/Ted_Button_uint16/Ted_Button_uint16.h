#ifndef _TED_BUTTON_UINT16_H
#define _TED_BUTTON_UINT16_H
/*
  Define a new button class derived from class Ted_Button, that attaches a
  uint16_t unsigned integer value to the button, its value being used as the
  button label.
*/

#include <Ted_Button.h>
#include <Ted_ArrowButton.h>

class Ted_Button_uint16 : public Ted_Button {

protected:
  uint16_t _value, _minValue, _maxValue;
  const char* _zeroString;
  uint16_t (*_checkValue)(Ted_Button_uint16& btn, uint16_t value);

public:
  /**************************************************************************/
  /*!
   @brief    Constructor.
   @param    name         String giving a name to the button, for debugging purposes only!
   @param    (others)     Remaining (optional) arguments are the same as initButton() below
  */
  /**************************************************************************/
  Ted_Button_uint16(const char* name, Adafruit_GFX* gfx=0, const char* align="C",
                  int16_t x=0, int16_t y=0, int16_t w=0, int16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint16_t textColor=0, const char* textAlign="C",
                  uint8_t textSize_x=1, uint8_t textSize_y=1,
                  const GFXfont* f=NULL, int16_t rCorner=0,
                  uint16_t value=0, uint16_t minValue=0, uint16_t maxValue=0,
                  const char* zeroString=NULL, bool degreeSym=false,
                  uint16_t (*checkValue)(Ted_Button_uint16& btn, uint16_t value) = NULL,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0) :
            Ted_Button(name) {
    initButton(gfx, align, x, y, w, h, outlineColor, fillColor, textColor, textAlign,
      textSize_x, textSize_y, f, rCorner, value, minValue, maxValue, zeroString,
      degreeSym, checkValue, expU, expD, expL, expR);
  }

  /**************************************************************************/
  /*!
   @brief   Initialize button with our desired color/size/etc. settings
   @param   gfx     See Ted_Button_Base::initButton()
   @param   align   See Ted_Button::initButton()
   @param   x       See Ted_Button::initButton()
   @param   y       See Ted_Button::initButton()
   @param   w       See Ted_Button::initButton()
   @param   h       See Ted_Button::initButton()
   @param   outlineColor  See Ted_Button_Base::initButton()
   @param   fillColor     See Ted_Button_Base::initButton()
   @param   textColor     See Ted_Button::initButton()
   @param   textAlign     See Ted_Button::initButton()
   @param   textSize_x    See Ted_Button::initButton()
   @param   textSize_y    See Ted_Button::initButton()
   @param   f             See Ted_Button::initButton()
   @param   rCorner       See Ted_Button::initButton()
   @param   value         The uint16_t value for the button, used to create the
                          label string for the button
   @param   minValue      The minimum allowed value for value
   @param   maxValue      The maximum allowed value for value
   @param   zeroString    If not NULL, this points to a character string to be
                          used as the button label if value is 0.
   @param   degreeSym     If true, a degree symbol is drawn at the end of the label
   @param   checkValue    If not NULL, a pointer to a function that checks a new
                          button value and can return an adjusted value if it is
                          out of range
   @param   expU    See Ted_Button_Base::initButton()
   @param   expD    See Ted_Button_Base::initButton()
   @param   expL    See Ted_Button_Base::initButton()
   @param   expR    See Ted_Button_Base::initButton()
  */
  /**************************************************************************/
  void initButton(Adafruit_GFX* gfx=0, const char* align="C",
                  int16_t x=0, int16_t y=0, int16_t w=0, int16_t h=0,
                  uint16_t outlineColor=0, uint16_t fillColor=0,
                  uint16_t textColor=0, const char* textAlign="C",
                  uint8_t textSize_x=1, uint8_t textSize_y=1,
                  const GFXfont* f=NULL, int16_t rCorner=0,
                  uint16_t value=0, uint16_t minValue=0, uint16_t maxValue=0,
                  const char* zeroString=NULL, bool degreeSym=false,
                  uint16_t (*checkValue)(Ted_Button_uint16& btn, uint16_t value) = NULL,
                  uint8_t expU=0, uint8_t expD=0, uint8_t expL=0, uint8_t expR=0);

  /**************************************************************************/
  /*!
   @brief   Get current button label uint16_t value
   @returns The current uint16_t value of the button
  */
  /**************************************************************************/
  uint16_t getValue(void) { return(_value); }

  /**************************************************************************/
  /*!
   @brief   Set button label uint16_t value, limiting the result to lie between
            minVal and maxVal and checking its value with checkValue function
            if one was supplied, convert the value to a string, and set that as
            the new button label, but don't redraw the button
   @param   value   The uint16_t value for the button
   @param   dontCheck   If true, don't call the checkValue function if it is
                        supplied, which may be necessary during initialization
                        when checkValue tests for interaction of two different
                        values, one of which may not yet have been initialized
   @returns true if the value is different than before
  */
  /**************************************************************************/
  bool setValue(uint16_t value, bool dontCheck = false);

  /**************************************************************************/
  /*!
   @brief   Set button label uint16_t value, convert the value to a string and
            set that as the new button label, then draw the button if the label
            changed or if any visible button attribute changed since last drawn.
   @param   value   The uint16_t value for the button
   @param   forceDraw   If true, the button is drawn even if attributes have
              not changed.
   @returns true if button was drawn
  */
  /**************************************************************************/
  bool setValueAndDrawIfChanged(uint16_t value, bool forceDraw = false);

  /**************************************************************************/
  /*!
   @brief   Add N to the button's value, limiting the result to lie between
            minVal and maxVal and using setValue to set the value, ensuring
            the value is fully checked. If btn is not NULL and btn->delta()
            is not 0, that value is used instead of N. Set the new value as the
            new label for the button and redraw it if the label or visible
            button attribute changed.

   @param   N     Value to add to this button's value, used if btn is NULL or
                  btn->delta() is 0
   @param   btn   Pointer to the base class of the button object that was
                  pressed in order to invoke this function, or NULL if none
   @returns true if button was drawn
  */
  /**************************************************************************/
  bool valueIncDec(int16_t N = 1, Ted_Button_Base* btn = NULL);

  };

#endif // _TED_BUTTON_UINT16_H
