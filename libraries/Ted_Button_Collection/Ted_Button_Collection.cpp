#include "Ted_Button_Collection.h"

/**************************************************************************/

void Ted_Button_Collection::clear() {
  release();
  numRegisteredButtons = 0;
  for (int i = 0; i < MAX_BUTTONS_IN_COLLECTION; i++) {
    buttons[i] = NULL;
    processPressFuncs[i] = NULL;
  }
  pressedButton = MAX_BUTTONS_IN_COLLECTION;
}

/**************************************************************************/

bool Ted_Button_Collection::registerButton(Ted_Button_Base& button,
    void (*processPress)(Ted_Button_Base& button)) {
  for (int i = 0; i < numRegisteredButtons; i++)
    if (buttons[i] == &button)
      return(true);
  if (numRegisteredButtons == MAX_BUTTONS_IN_COLLECTION)
    return(false);
  buttons[numRegisteredButtons] = &button;
  processPressFuncs[numRegisteredButtons] = processPress;
  numRegisteredButtons++;
  return(true);
}

/**************************************************************************/

bool Ted_Button_Collection::unregisterButton(Ted_Button_Base& button) {
  for (int i = 0; i < numRegisteredButtons; i++) {
    if (buttons[i] == &button) {
      numRegisteredButtons--;
      while (i < numRegisteredButtons) {
        buttons[i] = buttons[i+1];
        processPressFuncs[i] = processPressFuncs[i+1];
      }
      buttons[i] = NULL;
      processPressFuncs[i] = NULL;
      return(true);
    }
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button_Collection::press(int16_t x, int16_t y) {
  for (int i = 0; i < numRegisteredButtons; i++) {
    if (buttons[i]->contains(x, y)) {
      if (!buttons[i]->isPressed()) {
        if (masterPressRelease != NULL)
          masterPressRelease(true);
        pressedButton = i;
        buttons[pressedButton]->press();
        (*processPressFuncs[pressedButton]) (*buttons[pressedButton]);
        return(true);
      }
      return(false);
    }
  }
  return(false);
}

/**************************************************************************/

bool Ted_Button_Collection::release() {
  if (masterPressRelease != NULL)
    masterPressRelease(false);
  if (pressedButton == MAX_BUTTONS_IN_COLLECTION)
    return(false);
  buttons[pressedButton]->release();
  pressedButton = MAX_BUTTONS_IN_COLLECTION;
  return(true);
}

// -------------------------------------------------------------------------
