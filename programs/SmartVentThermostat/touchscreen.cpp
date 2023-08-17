/*
  touchscreen.cpp
*/
#include <XPT2046_Touchscreen.h>
#include <monitor_printf.h>
#include "touchscreen.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// Constants.
/////////////////////////////////////////////////////////////////////////////////////////////

// Touchscreen pin definitions.
#define TOUCH_CS          A0
#define TOUCH_IRQ         A7

// Touchscreen parameters.
#define MIN_TOUCH_PRESSURE    5  // Minimum required force for touch event
#define MAX_RELEASE_PRESSURE  0  // Maximum allowed force for release event
#define TS_MIN_LONG   300
#define TS_MAX_LONG   3750
#define TS_MIN_SHORT  550
#define TS_MAX_SHORT  3600

// Milliseconds of touch before recognized, or absence of touch before absence recognized.
#define TS_DEBOUNCE_MS  20

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables.
/////////////////////////////////////////////////////////////////////////////////////////////

// Touchscreen object.
static XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Touchscreen rotation.
static uint8_t screenRotation;

// true if last touchscreen event was a touch event, false if it was a release event.
static bool lastTSeventWasTouch;

// Used to time debouncing of touches.
static ulong msTime = millis();

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize touchscreen.
/////////////////////////////////////////////////////////////////////////////////////////////
void initTouchscreen(uint8_t rotation) {
  screenRotation = rotation;
  lastTSeventWasTouch = false;
  touch.begin();
  touch.setRotation(rotation);
  touch.setThresholds(Z_THRESHOLD/3);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check for new touch screen touches and releases. Return 0 if none, 1 if press, 2 if
// release. When TS_TOUCH_EVENT is returned, (x, y) contains the touched point and pres is
// the pressure. If px and py are not NULL, the raw x and y values are returned there.
/////////////////////////////////////////////////////////////////////////////////////////////
eTouchEvent getTouchEvent(int16_t& x, int16_t& y, int16_t& pres, int16_t* px, int16_t* py) {
  eTouchEvent ret = TS_UNCERTAIN;
  TS_Point p = touch.getPoint();

  if (px != NULL)
    *px = p.x;
  if (py != NULL)
    *py = p.y;

  if (screenRotation == 1) {
    x = map(p.x, TS_MIN_LONG, TS_MAX_LONG, 320, 0);
    y = map(p.y, TS_MIN_SHORT, TS_MAX_SHORT, 240, 0);
  } else if (screenRotation == 2) {
    x = map(p.x, TS_MIN_SHORT, TS_MAX_SHORT, 240, 0);
    y = map(p.y, TS_MIN_LONG, TS_MAX_LONG, 320, 0);
  } else if (screenRotation == 3) {
    x = map(p.x, TS_MIN_LONG, TS_MAX_LONG, 0, 320);
    y = map(p.y, TS_MIN_SHORT, TS_MAX_SHORT, 0, 240);
  } else {
    x = map(p.x, TS_MIN_SHORT, TS_MAX_SHORT, 0, 240);
    y = map(p.y, TS_MIN_LONG, TS_MAX_LONG, 0, 320);
  }
  pres = p.z;

  bool currentTSeventIsTouch = lastTSeventWasTouch;
  if (pres >= MIN_TOUCH_PRESSURE) {
    currentTSeventIsTouch = true;
    ret = TS_TOUCH_PRESENT;
  } else if (pres <= MAX_RELEASE_PRESSURE) {
    currentTSeventIsTouch = false;
    ret = TS_NO_TOUCH;
  }

  // If no change from last detected event, restart debounce timer.
  if (lastTSeventWasTouch == currentTSeventIsTouch) {
    msTime = millis();
    return(ret);
  }

  // A change since the last event has occurred, don't register it until debounce timer has expired.
  if (millis() - msTime < TS_DEBOUNCE_MS)
    return(ret);

  // Event occurred and debounce time expired.  Restart debounce timer for timing the opposite event.
  msTime = millis();
  lastTSeventWasTouch = currentTSeventIsTouch;
  return(currentTSeventIsTouch ? TS_TOUCH_EVENT : TS_RELEASE_EVENT);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check for touch screen button press or release and show a message on monitor if so.
/////////////////////////////////////////////////////////////////////////////////////////////
void showTouchesAndReleases() {
  int16_t x, y, pres;
  switch (getTouchEvent(x, y, pres)) {
    case TS_TOUCH_EVENT:
      monitor.printf("Touch at %d,%d\n", x, y);
      break;
    case TS_RELEASE_EVENT:
      monitor.printf("Release\n");
      break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
