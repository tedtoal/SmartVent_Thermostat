#ifndef _TED_BUTTON_COLLECTION_H
#define _TED_BUTTON_COLLECTION_H
/*
  Class Ted_Button_Collection holds an array of pointers to Ted_Button_Base
  objects. It can search them to find which one contains a given point, and it
  can call a function registered along with the button to process the contained
  point (such as when the user clicks or touches the button).
*/

#include <Ted_Button_Base.h>

#define MAX_BUTTONS_IN_COLLECTION 20

class Ted_Button_Collection {

protected:
  uint8_t numRegisteredButtons;
  Ted_Button_Base* buttons[MAX_BUTTONS_IN_COLLECTION];
  void (*processPressFuncs[MAX_BUTTONS_IN_COLLECTION])(Ted_Button_Base& button);
  uint8_t pressedButton; // Set to MAX_BUTTONS_IN_COLLECTION if none.
  void (*masterPressRelease)(bool press);

public:
  /**************************************************************************/
  /*!
   @brief    Constructor
  */
  /**************************************************************************/
  Ted_Button_Collection(void (*masterPressReleaseFunc)(bool press) = NULL) {
    numRegisteredButtons = MAX_BUTTONS_IN_COLLECTION;
    pressedButton = MAX_BUTTONS_IN_COLLECTION;
    masterPressRelease = NULL;
    clear();
  }

  /**************************************************************************/
  /*!
   @brief    Clear all currently registered buttons from the collection
  */
  /**************************************************************************/
  void clear(void);

  /**************************************************************************/
  /*!
   @brief    Register a master button press/release processing function
   @param    masterPressReleaseFunc   Function to call at each press or release
                                      of a button with argument indicating a
                                      press (true) or release (false), e.g.
                                      this function might make a sound upon
                                      button press, use NULL for none
  */
  /**************************************************************************/
  void registerMasterProcessFunc(void (*masterPressReleaseFunc)(bool press) = NULL) {
    masterPressRelease = masterPressReleaseFunc;
  }

  /**************************************************************************/
  /*!
   @brief    Register a button and its "press processing function"
   @param    button         The button to be registered
   @param    processPress   The function to call when the button is pressed,
                            with its single argument being a reference to the
                            button object
   @returns  true if successful or button is already registered, false if
             collection is full
  */
  /**************************************************************************/
  bool registerButton(Ted_Button_Base& button,
        void (*processPress)(Ted_Button_Base& button));

  /**************************************************************************/
  /*!
   @brief    Unregister a previously-registered button
   @param    button         The button to be unregistered
   @returns  true if successful, false if button was not previously registered
  */
  /**************************************************************************/
  bool unregisterButton(Ted_Button_Base& button);

  /**************************************************************************/
  /*!
   @brief       Search registered buttons for one containing the point (x,y),
                and if found, call masterPressRelease() if not NULL, call the
                button's press() function, and then call the registered
                processPress() function for that button
   @param    x  The X coordinate to check
   @param    y  The Y coordinate to check
   @returns  false if no button contains (x,y), else true
  */
  bool press(int16_t x, int16_t y);

  /**************************************************************************/
  /*!
   @brief       Call masterPressRelease() if not NULL, then call the release()
                function for the last button pressed if any
   @returns  false if no button was previously pressed, else true
  */
  bool release();

};

#endif // _TED_BUTTON_COLLECTION_H
