/*
  SmartVentThermostat

  Implement a thermostat for controlling a Beutler SmartVent via an EcoJay controller,
  on an Arduino Nano 33 IoT on an A-Z Touch MKR board that has an LCD touchscreen.

  15-Mar-2023
  by Ted Toal
*/
#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>
#include <monitor_printf.h>
#include <SPI.h>
#include <wdt_samd21.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen_TT.h>
#include <TS_Display.h>
#include <floatToString.h>
#include <msToString.h>
#include "nonvolatileSettings.h"
#include "thermistorAndTemperature.h"
#include "pinSettings.h"
#include "screens.h"
#include "screenAdvanced.h"
#include "screenCalibration.h"
#include "screenCleaning.h"
#include "screenDebug.h"
#include "screenMain.h"
#include "screenSettings.h"
#include "screenSpecial.h"

// Define the port to be used by the global instance named "the_serial_monitor".
#if USE_MONITOR_PORT
#define MONITOR_PORT &Serial  // Enable printf output to the serial monitor port identified by variable "Serial".
#else
#define MONITOR_PORT NULL     // Disable printf output when using WITHOUT the IDE, USB port, and serial monitor.
#endif

// *************************************************************************************** //
// Testing modes.
// *************************************************************************************** //

// Set TEST_MODE to one of the following values to test screens:
//  0 - normal operating mode
//  1 - show main screen but show rather than acting on touches and releases
//  2 - thermistor testing
//  3 - touchscreen testing screen
//  4 - test SETTINGS screen
//  5 - test ADVANCED screen
//  6 - test CLEANING screen
//  7 - test SPECIAL screen
//  8 - test CALIBRATION screen
//  9 - test DEBUG screen
#define TEST_MODE 0

// *************************************************************************************** //
// Constants.
// *************************************************************************************** //

// Amount of time to wait between each reading of indoor and outdoor temperatures.
// The temperatures are stored in a buffer and the running average temperatures are
// the values used to control SmartVent and to display on the screen. The number of
// temperatures averaged is given in the thermistorAndTemperature.h file and is
// currently 30. If we read temperatures once every two seconds, then the running
// average is flushed over a period of one minute. That seems reasonable.
#define TEMPERATURE_READ_TIME_MS (2*1000)

// Amount of time to wait after user exits Settings screen or touches the touchscreen,
// before beginning to check for SmartVent on/off conditions.
// Note: I want this to be less than LCD_BACKLIGHT_AUTO_OFF_MS so that the user can
// make a change on the screen, then wait for this time and hear the HVAC system
// activate and see the change right on the screen too, before it goes dark.
#define USER_ACTIVITY_DELAY_MS (10*1000)

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

// Millisecond time at the last transition from touching the touchscreen to not touching it.
static uint32_t lastNoTouchTime;

// This timer serves for turning off the LCD backlight a bit after the user finishes
// using the touchscreen. Each time the user does a screen touch, the LCD backlight
// is turned on if it was off. Each time the user releases his touch on the screen,
// this variable is zeroed. At regular updates when the screen is not being touched,
// this is counted upwards by milliseconds to LCD_BACKLIGHT_AUTO_OFF_MS, and when it
// reaches that, the backlight is turned off and this stops counting and is not reset.
static uint32_t MSsinceLastTouchBeforeBacklight;

// In order to be able to update the value of MSsinceLastTouchBeforeBacklight
// periodically, the following variable records the milliseconds time (millis())
// at which the last update was made to it, so that when the next update time
// occurs, the number of elapsed milliseconds can be determined and used to do
// the update.
static uint32_t MSatLastBacklightTimerUpdate;

// Timer counting up to TEMPERATURE_READ_TIME_MS, at which time current indoor and
// outdoor temperatures are read and the running average temperature is updated on
// the display as needed. The timer is then restarted.
static uint32_t MSsinceLastReadOfTemperatures;

// Like MSatLastBacklightTimerUpdate but for MSsinceLastReadOfTemperatures.
static uint32_t MSatLastTemperatureReadTimerUpdate;

// Thermostats seem to wait for a bit after the user fiddles with settings,
// before starting any activity. We will do the same here. This variable is
// zeroed each time the user does a screen touch, and at regular updates it
// is counted upwards by milliseconds. User touches initially change the
// settings in the "userSettings" variable. When this timer counts up to
// USER_ACTIVITY_DELAY_MS, then the  "userSettings" variable is copied to
// "activeSettings" and this stops counting and is not reset.
static uint32_t MSsinceLastTouchBeforeUserSettingsActivated;

// Like MSatLastBacklightTimerUpdate but for MSsinceLastTouchBeforeUserSettingsActivated.
static uint32_t MSatLastActionTimerUpdate;

// *************************************************************************************** //
// Touch screen processing.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Master button press/release processing function. On press, start playing a tone, and on
// release, end the tone.
/////////////////////////////////////////////////////////////////////////////////////////////
static void buttonPressRelease(bool press) {
  playSound(press);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check for touch screen button press or release. If so, check to see if it is one of the
// current screen registered buttons, and if so, process that press or release.
// The LCD backlight on/off is handled here too.
/////////////////////////////////////////////////////////////////////////////////////////////
void processTouchesAndReleases() {
  int16_t x, y, pres;

  // Check for a button press or release.
  switch (ts_display->getTouchEvent(x, y, pres)) {

  // When screen is not being touched or uncertain, update no-touch-since timers for backlight and settings activation.
  case TS_NO_TOUCH:
  case TS_UNCERTAIN:
    //monitor.printf("released\n");

    // Update the LCD backlight timer when screen is not being touched.
    // When backlight is turned off, we also exit the Cleaning screen if we are in it.
    if (MSsinceLastTouchBeforeBacklight < LCD_BACKLIGHT_AUTO_OFF_MS) {
      uint32_t MS = millis();
      uint32_t elapsedMS = MS - MSatLastBacklightTimerUpdate;
      MSatLastBacklightTimerUpdate = MS;
      MSsinceLastTouchBeforeBacklight += elapsedMS;
      if (MSsinceLastTouchBeforeBacklight >= LCD_BACKLIGHT_AUTO_OFF_MS) {
        MSsinceLastTouchBeforeBacklight = LCD_BACKLIGHT_AUTO_OFF_MS;
        setBacklight(false);
        if (currentScreen == SCREEN_CLEANING) {
          currentScreen = SCREEN_MAIN;
          drawMainScreen();
        }
      }
    }
    break;

  // As long as a touch is present, reset timeout timers.
  case TS_TOUCH_PRESENT:
    //monitor.printf("pressed\n");
    MSsinceLastTouchBeforeBacklight = 0;
    MSatLastBacklightTimerUpdate = millis();
    MSsinceLastTouchBeforeUserSettingsActivated = 0;
    MSatLastActionTimerUpdate = millis();
    break;

  // Touch events turn on the backlight if off, else are processed as possible screen button presses.
  case TS_TOUCH_EVENT:
    //monitor.printf("Button press: %d,%d pres=%d   isPressed: %d\n", x, y, pres, btn_OffAutoOn.isPressed());
    if (!getBacklight())
      setBacklight(true);
    else
      screenButtons->press(x, y);
    break;

  // Release events reset the timeout timers and are also tested for possible
  // screen button release.
  case TS_RELEASE_EVENT:
    //monitor.printf("Button release   isPressed: %d\n", btn_OffAutoOn.isPressed());
    MSsinceLastTouchBeforeBacklight = 0;
    MSatLastBacklightTimerUpdate = millis();
    MSsinceLastTouchBeforeUserSettingsActivated = 0;
    MSatLastActionTimerUpdate = millis();
    screenButtons->release();
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function for testing touch screen accuracy. To use this, call this from loop(). This waits
// for screen touches and draws + signs at them and shows the raw x,y value at each point.
// A long press clears the screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void testTouchScreen() {
  int16_t x, y, pres, rx, ry;
  int16_t w = 5; // Length of one arm of +
  switch (ts_display->getTouchEvent(x, y, pres, &rx, &ry)) {
  case TS_TOUCH_EVENT:
    lcd->drawFastHLine(x-w, y, 2*w, BLACK);
    lcd->drawFastVLine(x, y-w, 2*w, BLACK);
    char s[15];
    sprintf(s, "%u,%u", rx, ry);
    lcd->setTextColor(BLACK);
    lcd->setTextSize(1, 1);
    lcd->setFont(fontTom.getFont());
    int16_t X, Y, dX, dY, XF, YF;
    uint16_t W, H;
    fontTom.getTextBounds(s, 0, 0, &dX, &dY, &W, &H, &XF, &YF);
    if (x+w+W < 240)
      X = x+w-dX;
    else
      X = x-w-W-dX;
    if (y < H/2)
      Y = -dY;
    else
      Y = y-H/2-dY;
    lcd->setCursor(X, Y);
    lcd->print(s);
    break;

  case TS_NO_TOUCH:
    lastNoTouchTime = millis();
    break;

  case TS_TOUCH_PRESENT:
    if (millis() - lastNoTouchTime > 2000) {
      lcd->fillScreen(WHITE);
      lastNoTouchTime = millis();
    }
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check for touch screen button press or release and show a message on monitor if so.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTouchesAndReleases() {
  int16_t x, y, pres;
  switch (ts_display->getTouchEvent(x, y, pres)) {
    case TS_TOUCH_EVENT:
      monitor.printf("Touch at %d,%d\n", x, y);
      break;
    case TS_RELEASE_EVENT:
      monitor.printf("Release\n");
      break;
  }
}

// *************************************************************************************** //
// State processing and SmartVent on/off.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Current temperature processing. If it is time to read temperatures, do it, updating the
// running average in curIndoorTemperature and curOutdoorTemperature.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateCurrentTemperatures(void) {
  // Update read-temperatures timer and read the temperatures when the time arrives.
  uint32_t MS = millis();
  uint32_t elapsedMS = MS - MSatLastTemperatureReadTimerUpdate;
  MSatLastTemperatureReadTimerUpdate = MS;
  MSsinceLastReadOfTemperatures += elapsedMS;
  if (MSsinceLastReadOfTemperatures >= TEMPERATURE_READ_TIME_MS) {
    MSsinceLastReadOfTemperatures = 0;
    readCurrentTemperatures();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Update RunTimeMS timer, turning off SmartVent when appropriate.
// Note: maximum run time and new-day-reset of run time is handled below in
// updateSmartVentOnOff() because it works with current temperatures.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateSmartVentRunTimer(void) {

  // Get elapsed time since last update.
  uint32_t MS = millis();
  uint32_t elapsedMS = MS - MSatLastRunTimerUpdate;
  MSatLastRunTimerUpdate = MS;
  uint32_t MaxRunTimeMS = activeSettings.MaxRunTimeHours * 3600000UL;

  // If in OFF mode or if SmartVent is currently off (ON or AUTO mode), leave
  // the run timer alone.
  if (activeSettings.SmartVentMode != MODE_OFF && getSmartVent()) {
    // Advance RunTimeMS, but don't let it exceed 99 hours, and if it reaches a
    // set maximum run time limit, turn SmartVent off and change the mode and
    // arm state appropriately.
    RunTimeMS += elapsedMS;
    if (RunTimeMS > 99*3600000UL)
      RunTimeMS = 99*3600000UL;
    if (MaxRunTimeMS > 0 && RunTimeMS >= MaxRunTimeMS) {
      RunTimeMS = MaxRunTimeMS;
      setSmartVent(false);
      // If mode is ON, change ArmState to ARM_ON_TIMEOUT.
      if (activeSettings.SmartVentMode == MODE_ON) {
        setArmState(ARM_ON_TIMEOUT);
        monitor.printf("Timer expire while running in ON, set to AWAIT_HOT\n");
      // Else mode is AUTO. Change arm state to ARM_AWAIT_HOT. RunTimeMS remains
      // non-zero and is shown on the display, allowing the user to see how much
      // run time has occurred and see that it has hit his limit. It will be
      // reset to 0 when the arm state exits ARM_AWAIT_HOT.
      } else {
        if (activeSettings.SmartVentMode == MODE_AUTO) {
          setArmState(ARM_AWAIT_HOT);
          monitor.printf("Timer expire while running in AUTO, set to AWAIT_HOT\n");
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check the conditions to see if the SmartVent should be turned on/off.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateSmartVentOnOff(void) {
  uint32_t MaxRunTimeMS = activeSettings.MaxRunTimeHours * 3600000UL;
  bool isTimeout;
  float indoorTempAdjusted, outdoorTempAdjusted, halfHysteresis;

  switch (activeSettings.SmartVentMode) {

  //    - If mode is OFF and SmartVent is on, turn it off.
  case MODE_OFF:
    if (getSmartVent()) {
      setSmartVent(false);
      setArmState(ARM_OFF);
    }
    break;

  //    - If mode is ON, check if run time has hit its limit and set SmartVent accordingly.
  case MODE_ON:
    isTimeout = (MaxRunTimeMS > 0 && RunTimeMS == MaxRunTimeMS);
    if (isTimeout == getSmartVent()) {
      // Timeout and SmartVent is on, or no timeout and SmartVent is off.
      // Change SmartVent state to the proper one for the timeout state.
      setSmartVent(!isTimeout);
      setArmState(!isTimeout ? ARM_ON : ARM_ON_TIMEOUT);
    }
    break;

  //    - Else mode is AUTO. Check the various AUTO-mode conditions to determine
  //        whether SmartVent should be on or off, and change SmartVent state if
  //        the conditions warrant.
  case MODE_AUTO:
    indoorTempAdjusted = curIndoorTemperature.Tf + (float)activeSettings.IndoorOffsetF;
    outdoorTempAdjusted = curOutdoorTemperature.Tf + (float)activeSettings.OutdoorOffsetF;
    halfHysteresis = (float)activeSettings.HysteresisWidth / 2;

    // If SmartVent is off and ArmState is ARM_AWAIT_ON and MaxRunTimeMS is 0 or is greater than
    // RunTimeMS, evaluate whether or not to turn SmartVent on.
    if (!getSmartVent() && ArmState == ARM_AWAIT_ON && (MaxRunTimeMS == 0 || MaxRunTimeMS > RunTimeMS)) {
      // The condition to turn SmartVent on is: if the indoor temperature is above the indoor
      // temperature setpoint plus hysteresis AND the outdoor temperature is at or below the
      // indoor temperature minus the on-delta value required for activation minus hysteresis.
      bool turnOn = (indoorTempAdjusted > ((float)activeSettings.TempSetpointOn + halfHysteresis)) &&
        (outdoorTempAdjusted <= (indoorTempAdjusted - (float)activeSettings.DeltaTempForOn - halfHysteresis));
      // If the turn-on condition was satisfied, turn SmartVent on and change ArmState to ARM_AUTO_ON.
      if (turnOn) {
        setSmartVent(true);
        setArmState(ARM_AUTO_ON);
      }
    }

    // If SmartVent is on, evaluate whether or not to keep it on or turn it off.
    if (getSmartVent()) {
      // The condition to keep SmartVent on is almost the same as the condition to turn it on,
      // except that the hysteresis is reversed.
      // Note that the hysteresis ensures that it doesn't flip-flop on and off repeatedly in a short time interval.
      bool keepOn = (indoorTempAdjusted > ((float)activeSettings.TempSetpointOn - halfHysteresis)) &&
        (outdoorTempAdjusted <= (indoorTempAdjusted - (float)activeSettings.DeltaTempForOn + halfHysteresis));
      // If the keep-on condition was NOT satisfied, turn SmartVent off and change ArmState to ARM_AWAIT_ON.
      // Note that SmartVent will turn on again if the turn-on condition is again met, and in that case, the
      // total on-time accumulates up to the maximum on time, if one was set.
      if (!keepOn) {
        setSmartVent(false);
        setArmState(ARM_AWAIT_ON);
      }
    // Else SmartVent is off. If the outdoor temperature is at or above the indoor
    // temperature plus the delta value required for recognizing the start of a new
    // day, clear RunTimeMS.
    } else {
      if (outdoorTempAdjusted >= indoorTempAdjusted + (int16_t) activeSettings.DeltaNewDayTemp) {
        RunTimeMS = 0;
        // If ArmState is ARM_AWAIT_HOT, change to ARM_AWAIT_ON.
        if (ArmState == ARM_AWAIT_HOT)
          setArmState(ARM_AWAIT_ON);
      }
    }
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// ArmState must change if activeSettings.SmartVentMode changes. Call this each time that
// might have happened.
// All the interactions between mode, state, and run timer are complex and it is hard to
// get this right for all situations. I've continued to work on it.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateArmState(void) {
  switch (activeSettings.SmartVentMode) {
  case MODE_OFF:
    // In OFF mode, the arm state should always be ARM_OFF.
    setArmState(ARM_OFF);
    break;
  case MODE_ON:
    // In ON mode, only change the arm state if it isn't already one of the arm
    // states used in ON mode. If in ARM_AWAIT_HOT state, the run timer has
    // timed out, so go to the ARM_ON_TIMEOUT state to maintain timed-out timer.
    // User can clear timer by tapping the arm button.
    if (ArmState == ARM_OFF || ArmState == ARM_AUTO_ON || ArmState == ARM_AWAIT_ON)
      setArmState(ARM_ON);
    else if (ArmState == ARM_AWAIT_HOT)
      setArmState(ARM_ON_TIMEOUT);
    break;
  case MODE_AUTO:
    // In AUTO mode, only change the arm state if it isn't already one of the
    // arm states used in AUTO mode. If coming from OFF (where timer was zeroed),
    // go to ARM_AWAIT_ON. If coming from ARM_ON_TIMEOUT, go to ARM_AWAIT_HOT to
    // maintain timed-out timer. User can clear timer by tapping the arm button.
    if (ArmState == ARM_OFF)
      setArmState(ARM_AWAIT_ON);
    else if (ArmState == ARM_ON_TIMEOUT)
      setArmState(ARM_AWAIT_HOT);
    else if (ArmState == ARM_ON)
      setArmState(ARM_AUTO_ON);
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check for time to update active settings from user settings, after sufficient time has
// elapsed since last screen touch.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateActiveSettings() {
  // Update the user settings activation timer. (It is reset to 0 each time screen is touched).
  // Note that the only time userSettings itself changes is when user hits the Save button to
  // exit from the Settings screen.
  if (MSsinceLastTouchBeforeUserSettingsActivated < USER_ACTIVITY_DELAY_MS) {
    uint32_t MS = millis();
    uint32_t elapsedMS = MS - MSatLastActionTimerUpdate;
    MSatLastActionTimerUpdate = MS;
    MSsinceLastTouchBeforeUserSettingsActivated += elapsedMS;
    if (MSsinceLastTouchBeforeUserSettingsActivated >= USER_ACTIVITY_DELAY_MS) {
      MSsinceLastTouchBeforeUserSettingsActivated = USER_ACTIVITY_DELAY_MS;
      if (writeNonvolatileSettingsIfChanged(userSettings))
        monitor.printf("Wrote settings to non-volatile memory\n");
      // User settings become the active settings.
      activeSettings = userSettings;
      updateArmState();
    }
  }
}

// *************************************************************************************** //
// Standard Arduino setup function.
// *************************************************************************************** //
void setup() {
  // Initialize for using the Arduino IDE serial monitor.
  monitor.begin(MONITOR_PORT);
  monitor.printf("**************** RESET ****************\n");

  // Initialize watchdog timer. It must be reset every 16K clock cycles. Does this mean the
  // main system clock?  And what is it running at?  Actually, testing shows that it is
  // 16K MILLISECONDS.  We'll use four seconds. (Longest thing during init is temperature init,
  // taking about 2 seconds).
  #if USE_MONITOR_PORT == 0
  wdt_init(WDT_CONFIG_PER_4K);
  #endif

  // Initialize some hardware pins: SmartVent relay off, backlight on.
  monitor.printf("initPins()\n");
  initPins();

  // Initialize for reading temperatures. This also initializes the ADC.
  monitor.printf("Temperature\n");
  initReadTemperature(PIN_ADC_CALIB, PIN_PWM_CALIB, PIN_AREF_OUT, CFG_ADC_MULT_SAMP_AVG, wdt_reset);

  // Initialize timers.
  monitor.printf("Timers\n");
  // Initialize timer for next read of temperatures.
  MSsinceLastReadOfTemperatures = 0;
  MSatLastTemperatureReadTimerUpdate = millis();
  // Initialize action-on-new-settings timer.
  MSsinceLastTouchBeforeUserSettingsActivated = USER_ACTIVITY_DELAY_MS;
  MSatLastActionTimerUpdate = millis();
  // Initialize backlight timer.
  MSsinceLastTouchBeforeBacklight = 0;
  MSatLastBacklightTimerUpdate = millis();
  // Initial no-touch timer.
  lastNoTouchTime = millis();

  // Initialize screen objects.
  monitor.printf("Screen objects\n");
  initScreens();

  // Read settings from flash-based EEPROM into activeSettings, then copy them to userSettings.
  // Initialize touchscreen calibration parameter defaults from current settings in ts_display.
  monitor.printf("Nonvolatile\n");
  ts_display->getTS_calibration(&settingDefaults.TS_LR_X, &settingDefaults.TS_LR_Y,
    &settingDefaults.TS_UL_X, &settingDefaults.TS_UL_Y);
  readNonvolatileSettings(activeSettings, settingDefaults);
  userSettings = activeSettings;
  ts_display->setTS_calibration(userSettings.TS_LR_X, userSettings.TS_LR_Y,
    userSettings.TS_UL_X, userSettings.TS_UL_Y);
  updateArmState();

  // Initialize all screens.
  monitor.printf("all screens\n");
  initMainScreen();
  initSettingsScreen();
  initAdvancedScreen();
  initCleaningScreen();
  initSpecialScreen();
  initCalibrationScreen();
  initDebugScreen();
  // Register master button press/release processing function.
  screenButtons->registerMasterProcessFunc(buttonPressRelease);

  // Show screen indicated by TEST_MODE.
  monitor.printf("show screen   TEST_MODE: %d\n", TEST_MODE);
  #if TEST_MODE == 3  // Touchscreen testing.
  lcd->fillScreen(WHITE);
  setBacklight(true);
  #elif TEST_MODE == 4
  currentScreen = SCREEN_SETTINGS;
  drawSettingsScreen();
  #elif TEST_MODE == 5
  currentScreen = SCREEN_ADVANCED;
  drawAdvancedScreen();
  #elif TEST_MODE == 6
  currentScreen = SCREEN_CLEANING;
  drawCleaningScreen();
  #elif TEST_MODE == 7
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
  #elif TEST_MODE == 8
  currentScreen = SCREEN_CALIBRATION;
  drawCalibrationScreen();
  #elif TEST_MODE == 9
  currentScreen = SCREEN_DEBUG;
  drawDebugScreen();
  #else
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
  #endif

  monitor.printf("Initialization done, setup() returning\n");
}

// *************************************************************************************** //
// Standard Arduino main loop function.
// *************************************************************************************** //
void loop() {

  // Processing depends on TEST_MODE.
  #if TEST_MODE == 1  // show main screen but show rather than acting on touches and releases
  showTouchesAndReleases();
  #elif TEST_MODE == 2 // thermistor testing
  readAndShowCurrentTemperatures();
  delay(2000);
  #elif TEST_MODE == 3 // touchscreen testing screen
  testTouchScreen();

  #else // normal operating mode

  // Process button presses/releases on current screen. This also handles the LCD backlight
  // auto on/off and the storing of userSettings in EEPROM and copying it to activeSettings,
  // all after no user activity for a while.
  processTouchesAndReleases();

  // Update active settings from user settings.
  updateActiveSettings();

  // Update current temperatures.
  updateCurrentTemperatures();

  // Update SmartVent timers.
  updateSmartVentRunTimer();

  // Check the conditions to see if the SmartVent should be turned on/off:
  updateSmartVentOnOff();

  // Call function to process things according to which screen is currently displayed.
  switch (currentScreen) {
  case SCREEN_MAIN:
    loopMainScreen();
    break;
  case SCREEN_SETTINGS:
    loopSettingsScreen();
    break;
  case SCREEN_ADVANCED:
    loopAdvancedScreen();
    break;
  case SCREEN_CLEANING:
    loopCleaningScreen();
    break;
  case SCREEN_SPECIAL:
    loopSpecialScreen();
    break;
  case SCREEN_CALIBRATION:
    loopCalibrationScreen();
    // Don't turn off backlight in calibration mode.
    MSsinceLastTouchBeforeBacklight = 0;
    break;
  case SCREEN_DEBUG:
    loopDebugScreen();
    // Don't turn off backlight in debug mode.
    MSsinceLastTouchBeforeBacklight = 0;
    break;
  }

  #endif // TEST_MODE

  // Finally, reset the watchdog timer.
  wdt_reset();
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
