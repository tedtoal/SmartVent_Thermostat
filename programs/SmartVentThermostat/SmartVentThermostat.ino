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
#include <Button_TT.h>
#include <Button_TT_label.h>
#include <Button_TT_uint8.h>
#include <Button_TT_uint16.h>
#include <Button_TT_int8.h>
#include <Button_TT_int16.h>
#include <Button_TT_arrow.h>
#include <Button_TT_collection.h>
#include "fontsAndColors.h"
#include "thermistorAndTemperature.h"
#include "nonvolatileSettings.h"

// Set this to 1 to enable debug output to the serial monitor.
// Set this to 0 when releasing code to use without the IDE, USB port, and serial monitor.
#define USE_MONITOR_PORT 0

// Define the port to be used by the global instance named "the_serial_monitor".
#if USE_MONITOR_PORT
#define MONITOR_PORT &Serial  // Enable printf output to the serial monitor port identified by variable "Serial".
#else
#define MONITOR_PORT NULL     // Disable printf output when using WITHOUT the IDE, USB port, and serial monitor.
#endif

// Default for _PWM_LOGLEVEL_ if not defined is 1, SAMD_PWM tries to log stuff to serial monitor.
// If USE_MONITOR_PORT is defined as 0, we define _PWM_LOGLEVEL_ as 0 too.
#if USE_MONITOR_PORT == 0
#define _PWM_LOGLEVEL_ 0
#else
#define _PWM_LOGLEVEL_ 1
#endif
#include <SAMD_PWM.h>

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

// SmartVent activation relay pin number (digital output).
#define SMARTVENT_RELAY 5
#define SMARTVENT_OFF   LOW
#define SMARTVENT_ON    HIGH

// Set these pin numbers to valid values for your system. See comments in
// calibSAMD_ADC_withPWM.h describing these pins. These pin numbers are each
// indexes into g_APinDescription[] in variant.cpp. The values shown here are
// my values used on my system:
//  PIN_ADC_CALIB   7 = D7 = PA06 = AIN6
//  PIN_PWM_CALIB   4 = D4 = PA07 = TCC1 ch 1
//  PIN_AREF_OUT    6 = D6 = PA04
#define PIN_ADC_CALIB 7
#define PIN_PWM_CALIB 4
#define PIN_AREF_OUT  6

// Beeper pin.
#define BEEPER_PIN        A3

// Pin definitions for the LCD display.
#define LCD_DC            2
#define LCD_CS            10
#define LCD_MOSI          11
#define LCD_MISO          12
#define LCD_SCLK          13
#define LCD_BACKLIGHT_LED A2
#define LCD_WIDTH_PIXELS  240
#define LCD_HEIGHT_PIXELS 320

// Touchscreen pin definitions.
#define TOUCH_CS          A0
#define TOUCH_IRQ         A7

// Frequency to play when user presses the touch screen.
#define TS_TONE_FREQ    3000
// Duty cycle of "tone" (a square wave) in percent.  0 turns it off.
#define TS_TONE_DUTY    50

// LCD parameters.
#define LCD_BACKLIGHT_OFF HIGH
#define LCD_BACKLIGHT_ON LOW
#define LCD_BACKLIGHT_AUTO_OFF_MS (45*1000)

// Names to use on the display for indoors and outdoors.
#define INDOOR_NAME "Indoor"
#define OUTDOOR_NAME "Outdoor"

// Strings to show for certain arm states (see enum eArmState) when SmartVent is in AUTO mode.
#define STR_AUTO_ON "Venting"       // For state ARM_AUTO_ON
#define STR_AWAIT_HOT "Wait Hot"    // For state ARM_AWAIT_HOT
#define STR_AWAIT_ON "Wait On"      // For state ARM_AWAIT_ON

// Set this to 0 to disable ADC multiple sampling and averaging, or a number X
// between 1 and 10 to average 2^X samples, e.g. 6 means average 2^6 = 64 samples.
// This is internal hardware-based averaging. It produces more stable ADC values.
#define CFG_ADC_MULT_SAMP_AVG 6

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

// Corner radius of buttons with rounded corners, in pixels.
#define RAD 10

// Somewhat "standard" button width and height.
#define BTN_WIDTH  110
#define BTN_HEIGHT 40

// Negative button width/height values to use when creating buttons for the LCD display.
// A negative width or height means that the button is sized for the initial text label
// supplied for it, with the absolute value of the negative width or height ADDED to the
// text size to get the full button size.
#define ZEW 0   // Zero edge width
#define TEW -1  // Tiny edge width
#define SEW -5  // Small edge width
#define MEW -10 // Medium edge width
#define LEW -20 // Large edge width
#define HEW -30 // Huge edge width

// Values for expU, expD, expL, expR for button constructors.
#define EXP_T 5   // Tiny expansion of button
#define EXP_S 10  // Small expansion of button
#define EXP_M 20  // Medium expansion of button
#define EXP_L 30  // Large expansion of button
#define EXP_H 50  // Huge expansion of button

// *************************************************************************************** //
// Variables.
// *************************************************************************************** //

// PWM object for sound from beeper.
static SAMD_PWM* sound;

// LCD object.
static Adafruit_ILI9341* lcd;

// Touchscreen object.
static XPT2046_Touchscreen* touch;

// Touchscreen-LCD object.
static TS_Display* ts_display;

// Button collection object to manage the buttons of the currently displayed screen.
static Button_TT_collection* screenButtons;

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

// Is SmartVent currently ON (true) or OFF (false)?
static bool smartVentOn;

// SmartVent run timer. This counts by milliseconds whenever SmartVent is on (via
// ON or AUTO mode). The count is clamped at a maximum of 99 hours, which should
// only happen if mode is ON and SmartVent is allowed to run for over 4 days straight.
// This resets back to zero when SmartVent mode is OFF or when the user's maximum
// SmartVent run time is reached in AUTO or ON mode.
static uint32_t RunTimeMS;

// Arm states for SmartVent AUTO mode.
//  ARM_OFF: whenever SmartVent mode is OFF.
//  ARM_ON: whenever SmartVent mode is ON.
//  ARM_AWAIT_ON: initial arming state when the AUTO mode is activated (which can
//    occur simply by cycling modes through OFF, ON, AUTO, but may change immediately
//    to ARM_AUTO_ON if the outdoor temperature is low enough). This state also
//    occurs when state is ARM_AWAIT_HOT and outdoor temperature is hot enough.
//  ARM_AUTO_ON: occurs when state is ARM_AWAIT_ON and indoor temperature becomes
//    >= SmartVent setpoint temperature and outdoor temperature becomes <= indoor
//    temperature - DeltaTempForOn. At that time, SmartVent is turned ON and a
//    run-time timer is started.
//  ARM_AWAIT_HOT occurs when the state is ARM_AUTO_ON and a maximum SmartVent
//    run time is set and the SmartVent run timer reaches that value. At that time,
//    SmartVent is turned off. Exit from this state to ARM_AWAIT_ON when outdoor
//    temperature becomes >= indoor temperature + DeltaNewDayTemp.
typedef enum _eArmState {
  ARM_OFF,          // SmartVent is off in OFF mode.
  ARM_ON,           // SmartVent is on in ON mode.
  ARM_AUTO_ON,      // SmartVent is on in AUTO mode
  ARM_AWAIT_HOT,    // Timed out running in auto, SmartVent now off in AUTO mode and waiting till outdoor temp >= indoor temp + DeltaNewDayTemp
  ARM_AWAIT_ON      // SmartVent now off in AUTO mode and waiting till indoor temp >= setpoint temp and outdoor temp <= indoor temp - DeltaTempForOn
} eArmState;

// SmartVent arm state.
static eArmState ArmState;

// Like MSatLastBacklightTimerUpdate but for RunTimeMS.
static uint32_t MSatLastRunTimerUpdate;

// Screens.
typedef enum _eScreen {
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_ADVANCED,
  SCREEN_CLEANING,
  SCREEN_SPECIAL,
  SCREEN_CALIBRATION,
  SCREEN_DEBUG
} eScreen;

// Current screen.
static eScreen currentScreen;

// Functions for drawing screens.
static void drawMainScreen();
static void drawSettingsScreen();
static void drawAdvancedScreen();
static void drawCleaningScreen();
static void drawSpecialScreen();
static void drawCalibrationScreen(bool drawSaveButton=false);
static void drawDebugScreen();

// *************************************************************************************** //
// State processing and SmartVent on/off.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Set ArmState.
/////////////////////////////////////////////////////////////////////////////////////////////
void setArmState(eArmState newState) {
  if (ArmState != newState) {
    ArmState = newState;
    monitor.printf("ArmState changed to %d\n", ArmState);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////
// ArmState must change if activeSettings.SmartVentMode changes. Call this each time that
// might have happened.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateArmState(void) {
  switch (activeSettings.SmartVentMode) {
  case MODE_OFF:
    setArmState(ARM_OFF);
    break;
  case MODE_ON:
    setArmState(ARM_ON);
    break;
  case MODE_AUTO:
    // In AUTO mode, SmartVent might be on or off and that affects valid arm state.
    if (ArmState != ARM_AUTO_ON && ArmState != ARM_AWAIT_ON && ArmState != ARM_AWAIT_HOT)
      setArmState(smartVentOn ? ARM_AUTO_ON : ARM_AWAIT_ON);
    break;
  }
}

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
/////////////////////////////////////////////////////////////////////////////////////////////
void updateSmartVentRunTimer(void) {

  // Get elapsed time since last update.
  uint32_t MS = millis();
  uint32_t elapsedMS = MS - MSatLastRunTimerUpdate;
  MSatLastRunTimerUpdate = MS;
  uint32_t MaxRunTimeMS = activeSettings.MaxRunTimeHours * 3600000UL;

  // If in OFF mode, keep the run timer reset to 0.
  if (activeSettings.SmartVentMode == MODE_OFF)
    RunTimeMS = 0;
  // Else if SmartVent is currently on, advance RunTimeMS, don't let it exceed 99 hours,
  // and if it reaches a set maximum run time limit, turn SmartVent off and change the
  // mode and arm state appropriately.
  else if (smartVentOn) {
    RunTimeMS += elapsedMS;
    if (RunTimeMS > 99*3600000UL)
      RunTimeMS = 99*3600000UL;
    if (MaxRunTimeMS > 0) {
      if (RunTimeMS >= MaxRunTimeMS) {
        RunTimeMS = MaxRunTimeMS;
        smartVentOn = false;
        digitalWrite(SMARTVENT_RELAY, SMARTVENT_OFF);
        // If mode is AUTO, change arm state to ARM_AWAIT_HOT. RunTimeMS remains non-zero
        // so it is shown on the display, allowing the user to see how much run time has
        // occurred and see that it has hit his limit. It will be reset to 0 when the arm
        // state exits ARM_AWAIT_HOT.
        if (activeSettings.SmartVentMode == MODE_AUTO) {
          setArmState(ARM_AWAIT_HOT);
          monitor.printf("Timer expire while running, set to AWAIT_HOT\n");
        // Else mode is ON. Forcibly change it to OFF. Change both userSettings and
        // activeSettings and store settings in non-volatile memory. This is currently the
        // ONLY time when a change is made to userSettings and activeSettings by the code
        // rather than by the user.
        } else {
          userSettings.SmartVentMode = MODE_OFF;
          activeSettings.SmartVentMode = MODE_OFF;
          writeNonvolatileSettingsIfChanged(userSettings);
          RunTimeMS = 0;
          setArmState(ARM_OFF);
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Check the conditions to see if the SmartVent should be turned on/off.
// Note: maximum run time is handled in updateSmartVentRunTimer() above.
/////////////////////////////////////////////////////////////////////////////////////////////
void updateSmartVentOnOff(void) {
  uint32_t MaxRunTimeMS = activeSettings.MaxRunTimeHours * 3600000UL;

  switch (activeSettings.SmartVentMode) {

  //    - If mode is OFF and SmartVent is on, turn it off.
  case MODE_OFF:
    if (smartVentOn) {
      smartVentOn = false;
      digitalWrite(SMARTVENT_RELAY, SMARTVENT_OFF);
      RunTimeMS = 0;
      setArmState(ARM_OFF);
    }
    break;

  //    - If mode is ON and SmartVent is off, turn it on.
  case MODE_ON:
    if (!smartVentOn) {
      smartVentOn = true;
      digitalWrite(SMARTVENT_RELAY, SMARTVENT_ON);
      // Clear run time when starting SmartVent in ON mode.
      RunTimeMS = 0;
      setArmState(ARM_ON);
    }
    break;

  //    - Else mode is AUTO.
  case MODE_AUTO:
    float indoorTempAdjusted = curIndoorTemperature.Tf + (float)activeSettings.IndoorOffsetF;
    float outdoorTempAdjusted = curOutdoorTemperature.Tf + (float)activeSettings.OutdoorOffsetF;
    float halfHysteresis = (float)activeSettings.HysteresisWidth / 2;

    // If SmartVent is off and ArmState is ARM_AWAIT_ON and MaxRunTimeMS is 0 or is greater than
    // RunTimeMS, evaluate whether or not to turn SmartVent on.
    if (!smartVentOn && ArmState == ARM_AWAIT_ON && (MaxRunTimeMS == 0 || MaxRunTimeMS > RunTimeMS)) {
      // The condition to turn SmartVent on is: if the indoor temperature is above the indoor
      // temperature setpoint plus hysteresis AND the outdoor temperature is at or below the
      // indoor temperature minus the on-delta value required for activation minus hysteresis.
      bool turnOn = (indoorTempAdjusted > ((float)activeSettings.TempSetpointOn + halfHysteresis)) &&
        (outdoorTempAdjusted <= (indoorTempAdjusted - (float)activeSettings.DeltaTempForOn - halfHysteresis));
      // If the turn-on condition was satisfied, turn SmartVent on and change ArmState to ARM_AUTO_ON.
      if (turnOn) {
        smartVentOn = true;
        digitalWrite(SMARTVENT_RELAY, SMARTVENT_ON);
        setArmState(ARM_AUTO_ON);
      }
    }

    // If SmartVent is off and the outdoor temperature is at or above the indoor temperature plus the
    // delta value required for recognizing the start of a new day, clear RunTimeMS.
    if (!smartVentOn && outdoorTempAdjusted >= indoorTempAdjusted + (int16_t) activeSettings.DeltaNewDayTemp) {
      RunTimeMS = 0;
      // If ArmState is ARM_AWAIT_HOT, change to ARM_AWAIT_ON.
      if (ArmState == ARM_AWAIT_HOT)
        setArmState(ARM_AWAIT_ON);
    }

    // If SmartVent is on, evaluate whether or not to keep it on or turn it off.
    if (smartVentOn) {
      // The condition to keep SmartVent on is almost the same as the condition to turn it on,
      // except that the hysteresis is reversed.
      // Note that the hysteresis ensures that it doesn't flip-flop on and off repeatedly in a short time interval.
      bool keepOn = (indoorTempAdjusted > ((float)activeSettings.TempSetpointOn - halfHysteresis)) &&
        (outdoorTempAdjusted <= (indoorTempAdjusted - (float)activeSettings.DeltaTempForOn + halfHysteresis));
      // If the keep-on condition was NOT satisfied, turn SmartVent off and change ArmState to ARM_AWAIT_ON.
      // Note that SmartVent will turn on again if the turn-on condition is again met, and in that case, the
      // total on-time accumulates up to the maximum on time, if one was set.
      if (!keepOn) {
        smartVentOn = false;
        digitalWrite(SMARTVENT_RELAY, SMARTVENT_OFF);
        setArmState(ARM_AWAIT_ON);
      }
    }
    break;
  }
}

// *************************************************************************************** //
// Main screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// MAIN SCREEN buttons and fields. Fields can be implemented as buttons, perhaps with the
// outline color the same as the background color.
//
// The Main screen shows:
//  -	indoor and outdoor temperature
//  -	SmartVent status on/off
//  -	SmartVent mode off/auto/on button
//  -	while RunTimerMS is not zero, it is shown
//  - while SmartVent mode is AUTO, the Arm state is shown on a button which, if pressed,
//      cycles the state, allowing the user to either initiate or cancel a wait for the
//      hot part of the day.
//  -	Settings button
//  -	Advanced button
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Smart("Smart");
static Button_TT_label label_Vent("Vent");
static Button_TT_label field_SmartVentOnOff("VentOnOff");
static Button_TT_label btn_OffAutoOn("AutoOnOff");
static Button_TT_int16 field_IndoorTemp("IndoorTemp");
static Button_TT_label label_IndoorTemp("Indoor");
static Button_TT_int16 field_OutdoorTemp("OutdoorTemp");
static Button_TT_label label_OutdoorTemp("Outdoor");
static Button_TT_label field_RunTimer("RunTimer");
static Button_TT_label btn_ArmState("ArmState");
static Button_TT_label btn_Settings("Settings");
static Button_TT_label btn_Advanced("Advanced");

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons. Don't use constructor to init because can't use
// monitor.printf() inside button initButton() function then, since monitor is
// not initialized until setup().
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_MainScreen(void) {
  label_Smart.initButton(lcd, "TR", 130, 3, SEW, SEW, CLEAR, CLEAR, RED,
    "C", "Smart", false, &font18B);
  label_Vent.initButton(lcd, "TL", 130, 3, SEW, SEW, CLEAR, CLEAR, BLUE,
    "C", "Vent", false, &font18B);

  field_SmartVentOnOff.initButton(lcd, "TL", 10, 50, TEW, TEW, WHITE, WHITE, OLIVE,
    "C", "OFF", false, &font24B);
  btn_OffAutoOn.initButton(lcd, "TR", 230, 45, ZEW, SEW, BLACK, PINK, BLACK,
    "C", "AUTO", false, &font18, RAD, EXP_M, EXP_M, EXP_M, EXP_M);

  field_IndoorTemp.initButton(lcd, "TC", 60, 115, TEW, TEW, WHITE, WHITE, RED,
    "C", &font24B, 0, 0, -99, 199, true);
  label_IndoorTemp.initButton(lcd, "TC", 60, 160, TEW, TEW, CLEAR, CLEAR, RED,
    "C", INDOOR_NAME, false, &font12B);

  field_OutdoorTemp.initButton(lcd, "TC", 175, 115, TEW, TEW, WHITE, WHITE, BLUE,
    "C", &font24B, 0, 0, -99, 199, true);
  label_OutdoorTemp.initButton(lcd, "TC", 175, 160, TEW, TEW, CLEAR, CLEAR, BLUE,
    "C", OUTDOOR_NAME, false, &font12B);

  field_RunTimer.initButton(lcd, "TL", 10, 220, TEW, TEW, WHITE, WHITE, DARKGREEN,
    "C", "00:12:48", false, &mono12B);
  btn_ArmState.initButton(lcd, "TR", 235, 205, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", STR_AWAIT_ON, false, &font12, RAD, EXP_M, EXP_M, 0, 0);

  btn_Settings.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Settings", false, &font12, RAD, EXP_M, EXP_M, 0, 0);
  btn_Advanced.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Advanced", false, &font12, RAD, EXP_M, EXP_M, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Main screen. These are defined later, because
// they are both used by and need the functions defined below.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_OffAutoOn(Button_TT& btn);
static void btnPress_ArmState(Button_TT& btn);
static void btnPress_Settings(Button_TT& btn);
static void btnPress_Advanced(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the fields for the indoor and output temperatures to new current values and draw them.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatures(bool forceDraw = false) {
  field_IndoorTemp.setValueAndDrawIfChanged(
    curIndoorTemperature.Tf_int16+activeSettings.IndoorOffsetF, forceDraw);
  field_OutdoorTemp.setValueAndDrawIfChanged(
    curOutdoorTemperature.Tf_int16+activeSettings.OutdoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for SmartVent ON/OFF to new current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showSmartVentOnOff(bool forceDraw = false) {
  field_SmartVentOnOff.setLabelAndDrawIfChanged(smartVentOn ? "ON" : "OFF", forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the button label for the SmartVent mode to new active value and draw the button.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showSmartVentModeButton(bool forceDraw = false) {
  const char* S;
  switch(userSettings.SmartVentMode) {
  case MODE_OFF:
    S = "OFF";
    break;
  case MODE_ON:
    S = "ON";
    break;
  case MODE_AUTO:
    S = "AUTO";
    break;
  }
  btn_OffAutoOn.setLabelAndDrawIfChanged(S, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the appropriate label in the RunTimer field.
// If RunTimerMS is zero, the RunTimer field is empty.
// If RunTimerMS is non-zero, its value is shown in the RunTimer field.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showHideSmartVentRunTimer(bool forceDraw = false) {
  char S[10];
  if (RunTimeMS > 0) {
    msToString(RunTimeMS, S, sizeof(S), true, true, true, 2);
    field_RunTimer.setLabelAndDrawIfChanged(S, forceDraw);
  } else {
    field_RunTimer.setLabelAndDrawIfChanged("", forceDraw);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the appropriate label in the ArmState button and enable or disable the button.
// If SmartVent mode is OFF or ON, the ArmState button is not shown.
// If SmartVent mode is AUTO, the ArmState button shows the current ArmState state.
// Note: require both the ACTIVE AND USER mode to be AUTO to show the button. Otherwise,
// things get confusing while user is changing the mode.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showHideSmartVentArmStateButton(bool forceDraw = false) {
  if (activeSettings.SmartVentMode != MODE_AUTO || userSettings.SmartVentMode != MODE_AUTO) {
    screenButtons->unregisterButton(btn_ArmState);
    btn_ArmState.setOutlineColor(WHITE);
    btn_ArmState.setFillColor(WHITE);
    btn_ArmState.setLabelAndDrawIfChanged("", forceDraw);
  } else {
    btn_ArmState.setOutlineColor(BLACK);
    btn_ArmState.setFillColor(PINK);
    const char* S;
    switch (ArmState) {
    case ARM_AUTO_ON:
      S = STR_AUTO_ON;
      break;
    case ARM_AWAIT_ON:
      S = STR_AWAIT_ON;
      break;
    case ARM_AWAIT_HOT:
      S = STR_AWAIT_HOT;
      break;
    default:
      S = "Error";
      monitor.printf("ArmState is %d, wrong!\n", ArmState);
      break;
    }
    btn_ArmState.setLabelAndDrawIfChanged(S, forceDraw);
    screenButtons->registerButton(btn_ArmState, btnPress_ArmState);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Main screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawMainScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);

  label_Smart.drawButton();
  label_Vent.drawButton();

  label_IndoorTemp.drawButton();
  label_OutdoorTemp.drawButton();
  btn_Settings.drawButton();
  screenButtons->registerButton(btn_Settings, btnPress_Settings);
  btn_Advanced.drawButton();
  screenButtons->registerButton(btn_Advanced, btnPress_Advanced);

  showTemperatures(true);
  showSmartVentOnOff(true);
  showSmartVentModeButton(true);
  screenButtons->registerButton(btn_OffAutoOn, btnPress_OffAutoOn);
  showHideSmartVentRunTimer(true);
  showHideSmartVentArmStateButton(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of SmartVent mode button on Main screen. This cycles the mode: Off -> Auto -> On
// The new value is IMMEDIATELY written to userSettings, there is no "SAVE" button.  It is
// still the case that a delay elapses before userSettings is copied to activeSettings.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_OffAutoOn(Button_TT& btn) {
  Button_TT_label& btnOffAutoOn((Button_TT_label&) btn);
  const char* p = btnOffAutoOn.getLabel();
  eSmartVentMode mode;
  if (strcmp(p, "OFF") == 0) {
    mode = MODE_AUTO;
  } else if (strcmp(p, "AUTO") == 0) {
    mode = MODE_ON;
  } else {
    mode = MODE_OFF;
  }
  userSettings.SmartVentMode = mode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Arm State button on Main screen. This cycles the ArmState state between
// ARM_AWAIT_HOT and ARM_AWAIT_ON. This function is only called when the button is visible,
// in AUTO mode, never in OFF or ON mode.
// Actions to take depending on ArmState transition and current SmartVent on/off state:
//    ARM_AUTO_ON (SmartVent is On) ---> ARM_AWAIT_HOT:
//      action: turn off SmartVent, leave RunTimeMS unchanged
//    ARM_AWAIT_ON (SmartVent is Off) ---> ARM_AWAIT_HOT:
//      action: none, leave RunTimeMS unchanged
//    ARM_AWAIT_HOT (SmartVent is Off) ---> ARM_AWAIT_ON:
//      action: set RunTimeMS = 0
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_ArmState(Button_TT& btn) {
  switch (ArmState) {
  case ARM_AUTO_ON:
    smartVentOn = false;
    digitalWrite(SMARTVENT_RELAY, SMARTVENT_OFF);
    setArmState(ARM_AWAIT_HOT);
    break;
  case ARM_AWAIT_ON:
    setArmState(ARM_AWAIT_HOT);
    break;
  case ARM_AWAIT_HOT:
    RunTimeMS = 0;
    setArmState(ARM_AWAIT_ON);
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of "Settings" button on Main screen. This switches to the Settings screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Settings(Button_TT& btn) {
  currentScreen = SCREEN_SETTINGS;
  drawSettingsScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of "Advanced" button on Main screen. This switches to the Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Advanced(Button_TT& btn) {
  currentScreen = SCREEN_ADVANCED;
  drawAdvancedScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Main screen when it is shown.  This is called from loop().
// Elements of the main screen that can change are redrawn here (only if they have actually
// changed).
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopMainScreen() {
  // Update current indoor and outdoor temperatures on the screen.
  showTemperatures();

  // Update SmartVent ON/OFF on the screen.
  showSmartVentOnOff();

  // Update SmartVent mode button text.
  showSmartVentModeButton();

  // Update SmartVent run timer on the screen.
  showHideSmartVentRunTimer();

  // Update ArmState/DisArmState button on the screen.
  showHideSmartVentArmStateButton();
}

// *************************************************************************************** //
// Settings screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// SETTINGS SCREEN buttons and fields.
//
// The Settings screen shows:
//  -	indoor setpoint temperature for SmartVent
//  - degrees (delta) of difference between indoor temperature and cooler outdoor
//      temperature to turn SmartVent on
//  - degrees (band) of hysteresis, width around both the indoor setpoint temperature
//      AND the difference between indoor and outdoor temperature, to turn SmartVent on/off
//  -	run time limit
//  - Cancel button
//  - Save button
//
// The above data is obtained from the userSettings variable. As long as the settings
// screen is active, the current user settings are stored in the button objects.
// When the user exits the Settings screen with the SAVE button, the current button
// object values are copied back to the userSettings variable.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Settings("SettingsScreen");
static Button_TT_label label_TempSetpointOn("Setpoint1");
static Button_TT_int16 field_TempSetpointOn("Setpoint");
static Button_TT_arrow btn_TempSetpointOnLeft("SetpointLeft");
static Button_TT_arrow btn_TempSetpointOnRight("SetpointRight");
static Button_TT_label label1_DeltaTempForOn("DeltaOn1");
static Button_TT_label label2_DeltaTempForOn("DeltaOn2");
static Button_TT_uint8 field_DeltaTempForOn("DeltaOn");
static Button_TT_arrow btn_DeltaTempForOnLeft("DeltaLeft");
static Button_TT_arrow btn_DeltaTempForOnRight("DeltaRight");
static Button_TT_label label_Hysteresis1("Hysteresis1");
static Button_TT_label label_Hysteresis2("Hysteresis2");
static Button_TT_uint8 field_HysteresisWidth("Hysteresis");
static Button_TT_arrow btn_HysteresisWidthLeft("HysteresisLeft");
static Button_TT_arrow btn_HysteresisWidthRight("HysteresisRight");
static Button_TT_label label_MaxRun1("MaxRun1");
static Button_TT_label label_MaxRun2("MaxRun2");
static Button_TT_uint8 field_MaxRunTime("MaxRunTime");
static Button_TT_arrow btn_MaxRunTimeLeft("MaxRunTimeLeft");
static Button_TT_arrow btn_MaxRunTimeRight("MaxRunTimeRight");
static Button_TT_label label_MaxRun3("MaxRun3");
static Button_TT_label btn_SettingsCancel("SettingsCancel");
static Button_TT_label btn_SettingsSave("SettingsSave");

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_SettingsScreen(void) {
  label_Settings.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Settings", false, &font18B);

  label_TempSetpointOn.initButton(lcd, "TL", 5, 63, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Indoor", false, &font9B);
  field_TempSetpointOn.initButton(lcd, "TR", 150, 59, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_SETPOINT, MAX_TEMP_SETPOINT, true, false);
  btn_TempSetpointOnLeft.initButton(lcd, 'L', "TL", 158, 50, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_TempSetpointOnRight.initButton(lcd, 'R', "TL", 195, 50, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label1_DeltaTempForOn.initButton(lcd, "TL", 5, 101, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Outdoor", false, &font9B);
  label2_DeltaTempForOn.initButton(lcd, "TL", 5, 121, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "lower by", false, &font9B);
  field_DeltaTempForOn.initButton(lcd, "TR", 150, 107, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_DIFFERENTIAL, MAX_TEMP_DIFFERENTIAL, true);
  btn_DeltaTempForOnLeft.initButton(lcd, 'L', "TL", 158, 98, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_DeltaTempForOnRight.initButton(lcd, 'R', "TL", 195, 98, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_Hysteresis1.initButton(lcd, "TL", 5, 148, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Overshoot", false, &font9B);
  label_Hysteresis2.initButton(lcd, "TL", 5, 168, 90, SEW, CLEAR, CLEAR, MAROON,
    "CR", "width", false, &font9B);
  field_HysteresisWidth.initButton(lcd, "TR", 150, 155, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, MIN_TEMP_HYSTERESIS, MAX_TEMP_HYSTERESIS, true);
  btn_HysteresisWidthLeft.initButton(lcd, 'L', "TL", 158, 146, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_HysteresisWidthRight.initButton(lcd, 'R', "TL", 195, 146, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_MaxRun1.initButton(lcd, "TL", 5, 216, 50, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Max", false, &font9B);
  label_MaxRun2.initButton(lcd, "TL", 5, 237, 50, SEW, CLEAR, CLEAR, MAROON,
    "CR", "Run", false, &font9B);
  field_MaxRunTime.initButton(lcd, "TL", 57, 222, TEW, TEW, WHITE, WHITE, NAVY,
    "CR", &font18B, 0, 0, 0, MAX_RUN_TIME_IN_HOURS, false, MAX_RUN_TIME_0);
  btn_MaxRunTimeLeft.initButton(lcd, 'L', "TL", 105, 213, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_MaxRunTimeRight.initButton(lcd, 'R', "TL", 142, 213, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);
  label_MaxRun3.initButton(lcd, "TR", 230, 225, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "hours", false, &font9);

  btn_SettingsCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_SettingsSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Settings screen. These are defined later, because
// they are both used by and need the functions defined below.
// Note: the same function handles both the left and right arrow buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_TempSetpointOn(Button_TT& btn);
static void btnPress_DeltaTempForOn(Button_TT& btn);
static void btnPress_HysteresisWidth(Button_TT& btn);
static void btnPress_MaxRunTime(Button_TT& btn);
static void btnPress_SettingsCancel(Button_TT& btn);
static void btnPress_SettingsSave(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the temperature setpoint to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatureSetpoint(bool forceDraw = false) {
  field_TempSetpointOn.setValueAndDrawIfChanged(userSettings.TempSetpointOn, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the fields for the differential temperature settings to current values and draw them.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showTemperatureDifferentials(bool forceDraw = false) {
  field_DeltaTempForOn.setValueAndDrawIfChanged(userSettings.DeltaTempForOn, forceDraw);
  field_HysteresisWidth.setValueAndDrawIfChanged(userSettings.HysteresisWidth, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the maximum SmartVent run time to the current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showMaxRunTime(bool forceDraw = false) {
  field_MaxRunTime.setValueAndDrawIfChanged(userSettings.MaxRunTimeHours, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Settings screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawSettingsScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Settings.drawButton();

  lcd->drawRoundRect(2, 46, 236, 149, 5, BLACK);

  label_TempSetpointOn.drawButton();
  btn_TempSetpointOnLeft.drawButton();
  btn_TempSetpointOnRight.drawButton();
  screenButtons->registerButton(btn_TempSetpointOnLeft, btnPress_TempSetpointOn);
  screenButtons->registerButton(btn_TempSetpointOnRight, btnPress_TempSetpointOn);
  showTemperatureSetpoint(true);

  label1_DeltaTempForOn.drawButton();
  label2_DeltaTempForOn.drawButton();
  btn_DeltaTempForOnLeft.drawButton();
  btn_DeltaTempForOnRight.drawButton();
  screenButtons->registerButton(btn_DeltaTempForOnLeft, btnPress_DeltaTempForOn);
  screenButtons->registerButton(btn_DeltaTempForOnRight, btnPress_DeltaTempForOn);

  label_Hysteresis1.drawButton();
  label_Hysteresis2.drawButton();
  btn_HysteresisWidthLeft.drawButton();
  btn_HysteresisWidthRight.drawButton();
  screenButtons->registerButton(btn_HysteresisWidthLeft, btnPress_HysteresisWidth);
  screenButtons->registerButton(btn_HysteresisWidthRight, btnPress_HysteresisWidth);

  showTemperatureDifferentials(true);

  lcd->drawRoundRect(2, 209, 236, 53, 5, BLACK);

  label_MaxRun1.drawButton();
  label_MaxRun2.drawButton();
  btn_MaxRunTimeLeft.drawButton();
  btn_MaxRunTimeRight.drawButton();
  screenButtons->registerButton(btn_MaxRunTimeLeft, btnPress_MaxRunTime);
  screenButtons->registerButton(btn_MaxRunTimeRight, btnPress_MaxRunTime);
  label_MaxRun3.drawButton();
  showMaxRunTime(true);

  btn_SettingsCancel.drawButton();
  screenButtons->registerButton(btn_SettingsCancel, btnPress_SettingsCancel);
  btn_SettingsSave.drawButton();
  screenButtons->registerButton(btn_SettingsSave, btnPress_SettingsSave);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Settings screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for temperature setpoint.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_TempSetpointOn(Button_TT& btn) {
  field_TempSetpointOn.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for SmartVent-On temperature differential.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_DeltaTempForOn(Button_TT& btn) {
  field_DeltaTempForOn.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for SmartVent-Off temperature differential.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_HysteresisWidth(Button_TT& btn) {
  field_HysteresisWidth.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for maximum run time.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_MaxRunTime(Button_TT& btn) {
  field_MaxRunTime.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Settings screen. We switch to Main screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_SettingsCancel(Button_TT& btn) {
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Settings screen. We save settings from the buttons into
// "userSettings" and switch to the Main screen. The values will be copied from userSettings
// to activeSettings after sufficient time passes following the last button press.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_SettingsSave(Button_TT& btn) {
  userSettings.TempSetpointOn = field_TempSetpointOn.getValue();
  userSettings.DeltaTempForOn = field_DeltaTempForOn.getValue();
  userSettings.HysteresisWidth = field_HysteresisWidth.getValue();
  const char* pValue = field_MaxRunTime.getLabel();
  if (strcmp(pValue, MAX_RUN_TIME_0) == 0)
    userSettings.MaxRunTimeHours = 0;
  else
    userSettings.MaxRunTimeHours = (uint8_t) atoi(pValue);
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Settings screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopSettingsScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// Advanced screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// ADVANCED SCREEN buttons and fields.
//
// The Advanced screen shows:
//  - delta arm temperature for rearming
//  -	calibration delta temperatures for indoors and outdoors
//  - Cleaning button
//  - Special button
//  - Cancel button
//  - Save button
//
// The delta arm temperature and calibration delta temperatures are obtained from the
// userSettings variable. As long as the Advanced screen is active, the current user settings
// are stored in the button objects. When the user exits the Advanced screen with the SAVE
// button, the current button object values are copied back to the userSettings variable.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Advanced("AdvancedScreen");
static Button_TT_label label_DeltaNewDayTemp("DeltaArm1");
static Button_TT_uint8 field_DeltaNewDayTemp("DeltaArm");
static Button_TT_arrow btn_DeltaNewDayTempLeft("DeltaArmLeft");
static Button_TT_arrow btn_DeltaNewDayTempRight("DeltaArmRight");
static Button_TT_label label_IndoorOffset1("IndoorOffset1");
static Button_TT_label label_IndoorOffset2("IndoorOffset2");
static Button_TT_int8 field_IndoorOffset("IndoorOffset");
static Button_TT_arrow btn_IndoorOffsetLeft("IndoorLeft");
static Button_TT_arrow btn_IndoorOffsetRight("IndoorRight");
static Button_TT_label label_Outdoor("OutdoorOffset1");
static Button_TT_label label_OutdoorOffset("OutdoorOffset2");
static Button_TT_int8 field_OutdoorOffset("OutdoorOffset");
static Button_TT_arrow btn_OutdoorOffsetLeft("OutdoorOffsetLeft");
static Button_TT_arrow btn_OutdoorOffsetRight("OutdoorOffsetRight");
static Button_TT_label btn_Cleaning("Cleaning");
static Button_TT_label btn_Special("Special");
static Button_TT_label btn_AdvancedCancel("AdvancedCancel");
static Button_TT_label btn_AdvancedSave("AdvancedSave");

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_AdvancedScreen(void) {
  label_Advanced.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Advanced", false, &font18B);

  label_DeltaNewDayTemp.initButton(lcd, "TL", 5, 63, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Arm Diff", false, &font9B);
  field_DeltaNewDayTemp.initButton(lcd, "TL", 90, 59, TEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, 1, MAX_DELTA_ARM_TEMP, true);
  btn_DeltaNewDayTempLeft.initButton(lcd, 'L', "TL", 158, 50, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_DeltaNewDayTempRight.initButton(lcd, 'R', "TL", 195, 50, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_IndoorOffset1.initButton(lcd, "TL", 5, 117, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Indoor", false, &font9B);
  label_IndoorOffset2.initButton(lcd, "TL", 5, 138, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "offset", false, &font9B);
  field_IndoorOffset.initButton(lcd, "TL", 90, 123, SEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, -MAX_TEMP_CALIB_DELTA, MAX_TEMP_CALIB_DELTA, true, true);
  btn_IndoorOffsetLeft.initButton(lcd, 'L', "TL", 158, 114, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_IndoorOffsetRight.initButton(lcd, 'R', "TL", 195, 114, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  label_Outdoor.initButton(lcd, "TL", 5, 165, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "Outdoor", false, &font9B);
  label_OutdoorOffset.initButton(lcd, "TL", 5, 186, SEW, SEW, CLEAR, CLEAR, MAROON,
    "C", "offset", false, &font9B);
  field_OutdoorOffset.initButton(lcd, "TL", 90, 171, SEW, TEW, WHITE, WHITE, NAVY,
    "C", &font18B, 0, 0, -MAX_TEMP_CALIB_DELTA, MAX_TEMP_CALIB_DELTA, true, true);
  btn_OutdoorOffsetLeft.initButton(lcd, 'L', "TL", 158, 162, 43, 37, BLACK, PINK,
    0, 0, EXP_H, 0);
  btn_OutdoorOffsetRight.initButton(lcd, 'R', "TL", 195, 162, 43, 37, BLACK, PINK,
    0, 0, 0, EXP_H);

  btn_Cleaning.initButton(lcd, "TL", 5, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cleaning", false, &font12, RAD);
  btn_Special.initButton(lcd, "TR", 235, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Special", false, &font12, RAD);

  btn_AdvancedCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_AdvancedSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Advanced screen. These are defined later, because
// they are both used by and need the functions defined below.
// Note: the same function handles both the left and right arrow buttons.
/////////////////////////////////////////////////////////////////////////////////////////////

static void btnPress_DeltaNewDayTemp(Button_TT& btn);
static void btnPress_IndoorOffset(Button_TT& btn);
static void btnPress_OutdoorOffset(Button_TT& btn);
static void btnPress_Cleaning(Button_TT& btn);
static void btnPress_Special(Button_TT& btn);
static void btnPress_AdvancedCancel(Button_TT& btn);
static void btnPress_AdvancedSave(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the delta arm temperature to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showDeltaNewDayTemp(bool forceDraw = false) {
  field_DeltaNewDayTemp.setValueAndDrawIfChanged(userSettings.DeltaNewDayTemp, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the indoor offset to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showIndoorOffset(bool forceDraw = false) {
  field_IndoorOffset.setValueAndDrawIfChanged(userSettings.IndoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Set the field for the outdoor offset to current value and draw it.
/////////////////////////////////////////////////////////////////////////////////////////////
static void showOutdoorOffset(bool forceDraw = false) {
  field_OutdoorOffset.setValueAndDrawIfChanged(userSettings.OutdoorOffsetF, forceDraw);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Advanced screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawAdvancedScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Advanced.drawButton();

  lcd->drawRoundRect(2, 46, 236, 53, 5, BLACK);

  label_DeltaNewDayTemp.drawButton();
  btn_DeltaNewDayTempLeft.drawButton();
  btn_DeltaNewDayTempRight.drawButton();
  screenButtons->registerButton(btn_DeltaNewDayTempLeft, btnPress_DeltaNewDayTemp);
  screenButtons->registerButton(btn_DeltaNewDayTempRight, btnPress_DeltaNewDayTemp);
  showDeltaNewDayTemp(true);

  lcd->drawRoundRect(2, 110, 236, 102, 5, BLACK);

  label_IndoorOffset1.drawButton();
  label_IndoorOffset2.drawButton();
  btn_IndoorOffsetLeft.drawButton();
  btn_IndoorOffsetRight.drawButton();
  screenButtons->registerButton(btn_IndoorOffsetLeft, btnPress_IndoorOffset);
  screenButtons->registerButton(btn_IndoorOffsetRight, btnPress_IndoorOffset);
  showIndoorOffset(true);

  label_Outdoor.drawButton();
  label_OutdoorOffset.drawButton();
  btn_OutdoorOffsetLeft.drawButton();
  btn_OutdoorOffsetRight.drawButton();
  screenButtons->registerButton(btn_OutdoorOffsetLeft, btnPress_OutdoorOffset);
  screenButtons->registerButton(btn_OutdoorOffsetRight, btnPress_OutdoorOffset);
  showOutdoorOffset(true);

  btn_Cleaning.drawButton();
  screenButtons->registerButton(btn_Cleaning, btnPress_Cleaning);

  btn_Special.drawButton();
  screenButtons->registerButton(btn_Special, btnPress_Special);

  btn_AdvancedCancel.drawButton();
  screenButtons->registerButton(btn_AdvancedCancel, btnPress_AdvancedCancel);
  btn_AdvancedSave.drawButton();
  screenButtons->registerButton(btn_AdvancedSave, btnPress_AdvancedSave);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for delta arm temperature.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_DeltaNewDayTemp(Button_TT& btn) {
  field_DeltaNewDayTemp.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for indoor offset.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_IndoorOffset(Button_TT& btn) {
  field_IndoorOffset.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of left or right arrow for outdoor offset.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_OutdoorOffset(Button_TT& btn) {
  field_OutdoorOffset.valueIncDec(1, &btn);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cleaning button in Advanced screen. We switch to Cleaning screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Cleaning(Button_TT& btn) {
  currentScreen = SCREEN_CLEANING;
  drawCleaningScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Special button in Advanced screen. We switch to Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Special(Button_TT& btn) {
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Advanced screen. We switch to Main screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_AdvancedCancel(Button_TT& btn) {
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Advanced screen. We save settings from the buttons into
// "userSettings" AND INTO "activeSettings", so they take effect immediately, then switch to
// the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_AdvancedSave(Button_TT& btn) {
  userSettings.DeltaNewDayTemp = activeSettings.DeltaNewDayTemp = field_DeltaNewDayTemp.getValue();
  userSettings.IndoorOffsetF = activeSettings.IndoorOffsetF = field_IndoorOffset.getValue();
  userSettings.OutdoorOffsetF = activeSettings.OutdoorOffsetF = field_OutdoorOffset.getValue();
  currentScreen = SCREEN_MAIN;
  drawMainScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Advanced screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopAdvancedScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// Cleaning screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// CLEANING SCREEN buttons and fields.
//
// The Cleaning screen shows a message telling the user he can clean the screen and screen
// presses will be ignored. After he stops touching the screen for a while, it returns to
// the Main screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Cleaning("CleaningScreen");
static Button_TT_label label_CleanTheScreen("CleanTheScreen");
static Button_TT_label label_EndsAfter("EndsAfter");
static Button_TT_label label_NoActivity("NoActivity");

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_CleaningScreen(void) {
  label_Cleaning.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Cleaning", false, &font18B);
  label_CleanTheScreen.initButton(lcd, "CC", 120, 100, TEW, TEW, CLEAR, CLEAR, OLIVE,
    "C", "Clean the Screen", false, &font12B);
  label_EndsAfter.initButton(lcd, "TC", 120, 200, TEW, TEW, CLEAR, CLEAR, DARKGREY,
    "C", "Ends After", false, &font12B);
  label_NoActivity.initButton(lcd, "TC", 120, 230, TEW, TEW, CLEAR, CLEAR, DARKGREY,
    "C", "No Activity", false, &font12B);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Cleaning screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawCleaningScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);

  label_Cleaning.drawButton();
  label_CleanTheScreen.drawButton();
  label_EndsAfter.drawButton();
  label_NoActivity.drawButton();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Cleaning screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopCleaningScreen() {
  // No actions required. When LCD backlight timeout occurs, the Main screen will be reactivated.
}

// *************************************************************************************** //
// Special screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// SPECIAL SCREEN buttons and fields.
//
// The Special screen shows more buttons to enter additional specialized screens, currently
// the calibration screen and debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Special("SpecialScreen");
static Button_TT_label btn_Calibration("Calibration");
static Button_TT_label btn_Debug("Debug");
static Button_TT_label btn_SpecialDone("SpecialDone");

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Special screen. These are defined later, because
// they are both used by and need the functions defined below.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_SpecialDone(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_SpecialScreen(void) {
  label_Special.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Special", false, &font18B);

  btn_Cleaning.initButton(lcd, "TL", 5, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cleaning", false, &font12, RAD);
  btn_Special.initButton(lcd, "TR", 235, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Special", false, &font12, RAD);

  btn_Calibration.initButton(lcd, "TL", 5, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Calibrate", false, &font12, RAD);
  btn_Debug.initButton(lcd, "TR", 235, 223, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Debug", false, &font12, RAD);

  btn_SpecialDone.initButton(lcd, "BC", 120, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Done", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Special screen. These are defined later, because
// they are both used by and need the functions defined below.
/////////////////////////////////////////////////////////////////////////////////////////////

static void btnPress_Calibration(Button_TT& btn);
static void btnPress_Debug(Button_TT& btn);
static void btnPress_SpecialDone(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Special screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawSpecialScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Special.drawButton();

  btn_Calibration.drawButton();
  screenButtons->registerButton(btn_Calibration, btnPress_Calibration);

  btn_Debug.drawButton();
  screenButtons->registerButton(btn_Debug, btnPress_Debug);

  btn_SpecialDone.drawButton();
  screenButtons->registerButton(btn_SpecialDone, btnPress_SpecialDone);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Calibration button in Special screen. We switch to Calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Calibration(Button_TT& btn) {
  currentScreen = SCREEN_CALIBRATION;
  drawCalibrationScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Debug button in Special screen. We switch to Debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_Debug(Button_TT& btn) {
  currentScreen = SCREEN_DEBUG;
  drawDebugScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Done button in Special screen. We switch (back) to Advanced screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_SpecialDone(Button_TT& btn) {
  currentScreen = SCREEN_ADVANCED;
  drawAdvancedScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Special screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopSpecialScreen() {
  // No actions required. Button handler functions take care of everything.
}

// *************************************************************************************** //
// Calibration screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// CALIBRATION SCREEN buttons and fields.
//
// The Calibration screen initially shows a Cancel button, a message to touch the "+", and a
// single "+" in one corner. When Cancel is touched the screen exits without changing the
// calibration setting. If the "+" is touched, it is erased and a second "+" in the opposite
// corner is displayed. If that "+" is also touched, it is erased, calibration settings are
// recomputed and temporarily changed, a Save button is shown along with the Cancel button,
// a message is displayed to touch anywhere to test calibration, and subsequent touches cause
// a "+" of another color to be drawn at that position. Touching Cancel reverts to original
// calibration settings, while touching Save saves the new calibration settings in userSettings
// where they will be copied to activeSettings and saved to nonvolatile memory as usual after a
// delay, and the screen exits back to the Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Calibration("CalibrationScreen");
static Button_TT_label btn_CalibrationCancel("CalibrationCancel");
static Button_TT_label btn_CalibrationSave("CalibrationSave");

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_CalibrationScreen(void) {
  label_Calibration.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Calibrate", false, &font18B);

  btn_CalibrationCancel.initButton(lcd, "BL", 5, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Cancel", false, &font12, RAD);
  btn_CalibrationSave.initButton(lcd, "BR", 235, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Save", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Calibration screen. These are defined later,
// because they are both used by and need the functions defined below.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_CalibrationCancel(Button_TT& btn);
static void btnPress_CalibrationSave(Button_TT& btn);

// Length of each arm of "+" sign.
#define PLUS_ARM_LEN 10

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw a plus sign at a specified display location.
/////////////////////////////////////////////////////////////////////////////////////////////
void drawPlus(int16_t x, int16_t y, int16_t color, uint8_t len = PLUS_ARM_LEN) {
  lcd->drawFastVLine(x, y-len, 2*len+1, color);
  lcd->drawFastHLine(x-len, y, 2*len+1, color);
}

// States during calibration and subsequent showing of tapped points.
typedef enum _eCalibState {
  STATE_WAIT_UL,            // Wait for user to tap + at upper-left
  STATE_WAIT_UL_RELEASE,    // Wait for him to release the tap
  STATE_WAIT_LR,            // Wait for user to tap + at lower-right
  STATE_WAIT_LR_RELEASE,    // Wait for him to release the tap
  STATE_WAIT_POINT_SHOW_IT, // Wait for user to tap anywhere, then draw "+" there
  STATE_WAIT_RELEASE        // Wait for him to release the tap
} eCalibState;

// Current state of calibration screen interaction with user.
static eCalibState calibState;

// Display UL and LR calibration positions and corresponding touchscreen calibration coordinates.
static int16_t x_UL, y_UL, x_LR, y_LR;
static int16_t TSx_UL, TSy_UL, TSx_LR, TSy_LR;

/////////////////////////////////////////////////////////////////////////////////////////////
// Print string S to display at cursor position (x,y) in specified color.
/////////////////////////////////////////////////////////////////////////////////////////////
void lcd_print(int16_t x, int16_t y, int16_t color, const char* S) {
  lcd->setCursor(x, y);
  lcd->setTextColor(color);
  lcd->print(S);
}

// Text for user instructions to tap "+".
#define TEXT_TAP_PLUS "Tap the +"

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Calibration screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawCalibrationScreen(bool drawSaveButton) {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Calibration.drawButton();

  btn_CalibrationCancel.drawButton();
  screenButtons->registerButton(btn_CalibrationCancel, btnPress_CalibrationCancel);

  if (drawSaveButton) {
    btn_CalibrationSave.drawButton();
    screenButtons->registerButton(btn_CalibrationSave, btnPress_CalibrationSave);
  }

  // Get position of two display points at which to draw "+" signs and require that
  // the user tap them to calibrate the touchscreen.
  ts_display->GetCalibration_UL_LR(PLUS_ARM_LEN+2, &x_UL, &y_UL, &x_LR, &y_LR);

  // Paint first "+" and wait for user to tap that point.
  drawPlus(x_UL, y_UL, BLUE);
  lcd_print(20, 60, RED, TEXT_TAP_PLUS);
  calibState = STATE_WAIT_UL;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Calibration screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Cancel button in Calibration screen. We revert calibration parameters to
// the original and switch back to Special screen without save.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_CalibrationCancel(Button_TT& btn) {
  ts_display->setTS_calibration(userSettings.TS_LR_X, userSettings.TS_LR_Y,
    userSettings.TS_UL_X, userSettings.TS_UL_Y);
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Save button in Calibration screen. We save current calibration parameters
// into "userSettings" and switch back to the Special screen. The values will be copied from
// userSettings to activeSettings sufficient time passes following the last button press.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_CalibrationSave(Button_TT& btn) {
  ts_display->getTS_calibration(&userSettings.TS_LR_X, &userSettings.TS_LR_Y,
    &userSettings.TS_UL_X, &userSettings.TS_UL_Y);
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Calibration screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopCalibrationScreen() {
  boolean isTouched = touch->touched();
  TS_Point p;
  if (isTouched)
    p = touch->getPoint();

  switch (calibState) {

  case STATE_WAIT_UL:
    if (isTouched) {
      // Record the touchscreen coordinates.
      TSx_UL = p.x;
      TSy_UL = p.y;
      // Play sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, TS_TONE_DUTY);
      calibState = STATE_WAIT_UL_RELEASE;
    }
    break;

  case STATE_WAIT_UL_RELEASE:
    if (!isTouched) {
      // Erase the first plus and instructions.
      drawPlus(x_UL, y_UL, WHITE);
      lcd_print(20, 60, WHITE, TEXT_TAP_PLUS);
      // Paint second + and wait for user to tap that point.
      drawPlus(x_LR, y_LR, BLUE);
      lcd_print(120, 260, RED, TEXT_TAP_PLUS);
      // Stop sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, 0);
      calibState = STATE_WAIT_LR;
    }
    break;

  case STATE_WAIT_LR:
    if (isTouched) {
      // Record the touchscreen coordinates.
      TSx_LR = p.x;
      TSy_LR = p.y;
      // Play sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, TS_TONE_DUTY);
      calibState = STATE_WAIT_LR_RELEASE;
    }
    break;

  case STATE_WAIT_LR_RELEASE:
    if (!isTouched) {
      // Map the two touchscreen points to the correct calibration values at the
      // extreme ends of the display. Set resulting calibration parameters as the
      // new calibration parameters in ts_display.
      int16_t TS_LR_X, TS_LR_Y, TS_UL_X, TS_UL_Y;
      ts_display->findTS_calibration(x_UL, y_UL, x_LR, y_LR, TSx_UL, TSy_UL, TSx_LR, TSy_LR,
        &TS_LR_X, &TS_LR_Y, &TS_UL_X, &TS_UL_Y);
      ts_display->setTS_calibration(TS_LR_X, TS_LR_Y, TS_UL_X, TS_UL_Y);
      // Redraw the screen with "Save" button and show new instructions.
      drawCalibrationScreen(true);
      lcd_print(10, 200, RED, "Tap to test calibration");
      // Stop sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, 0);
      calibState = STATE_WAIT_POINT_SHOW_IT;
    }
    break;

  case STATE_WAIT_POINT_SHOW_IT:
    if (isTouched) {
      // Map touched point to display and draw a green "+" at that point.
      int16_t x, y;
      ts_display->mapTStoDisplay(p.x, p.y, &x, &y);
      drawPlus(x, y, DARKGREEN);
      // Play sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, TS_TONE_DUTY);
      calibState = STATE_WAIT_RELEASE;
    }
    break;

  case STATE_WAIT_RELEASE:
    if (!isTouched) {
      // Stop sound.
      sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, 0);
      calibState = STATE_WAIT_POINT_SHOW_IT;
    }
    break;

  }

  // Don't turn off backlight in calibration mode.
  MSsinceLastTouchBeforeBacklight = 0;
}

// *************************************************************************************** //
// Debug screen.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG SCREEN buttons and fields.
//
// The Debug screen shows debug info (currently, indoor temperature resistance values) with a
// "Done" button at the bottom.
/////////////////////////////////////////////////////////////////////////////////////////////
static Button_TT_label label_Debug("DebugScreen");
static Button_TT_label btn_DebugDone("DebugDone");

/////////////////////////////////////////////////////////////////////////////////////////////
// Constants and variables for managing the fields_DebugArea buttons, and to track the
// position of the next data to be added.
/////////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG_ROW_Y_HEIGHT 6 // TomThumb font yAdvance
#define DEBUG_ROW_Y_SPACING (DEBUG_ROW_Y_HEIGHT+2)
#define WIDTH_DEBUG_AREA   220
#define HEIGHT_DEBUG_AREA   220
#define NUM_ROWS_DEBUG_AREA ((uint8_t)(HEIGHT_DEBUG_AREA/DEBUG_ROW_Y_SPACING))
#define SPRINTF_FORMAT_THERMISTOR_R_ROW "%5d in:A=%-5d R=%-6d T=%-4s out:A=%-5d R=%-6d T=%-4s" // 60 + \0
#define LEN_THERMISTOR_R_ROW (5+1+5+5+1+2+6+1+2+4+1+6+5+1+2+6+1+2+4+1) // 60 + \0
static Button_TT_label* fields_DebugArea[NUM_ROWS_DEBUG_AREA];
static uint8_t rowIdx_DebugArea;
static uint16_t lastReadCount_DebugArea;

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize above buttons.
/////////////////////////////////////////////////////////////////////////////////////////////
static void initButtons_DebugScreen(void) {
  label_Debug.initButton(lcd, "TC", 120, 5, TEW, TEW, CLEAR, CLEAR, DARKGREEN,
    "C", "Debug", false, &font18B);

  uint16_t y = 40; // Starting y-position
  for (uint8_t i = 0; i < NUM_ROWS_DEBUG_AREA; i++) {
    fields_DebugArea[i] = new Button_TT_label("dbg");
    fields_DebugArea[i]->initButton(lcd, "TL", 10, y, WIDTH_DEBUG_AREA, DEBUG_ROW_Y_HEIGHT, WHITE, WHITE, NAVY,
      "C", "", false, &fontTom);
    y += DEBUG_ROW_Y_SPACING;
  }

  btn_DebugDone.initButton(lcd, "BC", 120, 313, BTN_WIDTH, BTN_HEIGHT, BLACK, PINK, BLACK,
    "C", "Done", false, &font12, RAD);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declare button handler functions for the Debug screen. These are defined later, because
// they are both used by and need the functions defined below.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_DebugDone(Button_TT& btn);

/////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if there is a new indoor thermistor R value available, and if so, add it to
// text_DebugArea and set the new label for field_DebugArea and draw the label if changed.
/////////////////////////////////////////////////////////////////////////////////////////////
static void updateDebugScreen() {
  if (lastReadCount_DebugArea != NtempReads) {
    lastReadCount_DebugArea = NtempReads;
    char S[LEN_THERMISTOR_R_ROW];
    char Tin[8];
    char Tout[8];
    floatToString(degCtoF(TlastIndoorTempRead), Tin, sizeof(Tin), 1);
    floatToString(degCtoF(TlastOutdoorTempRead), Tout, sizeof(Tout), 1);
    snprintf_P(S, LEN_THERMISTOR_R_ROW, SPRINTF_FORMAT_THERMISTOR_R_ROW,
      lastReadCount_DebugArea,
      ADClastIndoorTempRead, RlastIndoorTempRead, Tin,
      ADClastOutdoorTempRead, RlastOutdoorTempRead, Tout);
    fields_DebugArea[rowIdx_DebugArea]->setLabelAndDrawIfChanged(S);
    if (++rowIdx_DebugArea == NUM_ROWS_DEBUG_AREA)
      rowIdx_DebugArea = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Draw the Debug screen and register its buttons with the global screenButtons object.
/////////////////////////////////////////////////////////////////////////////////////////////
static void drawDebugScreen() {
  screenButtons->clear();

  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
  MSsinceLastTouchBeforeBacklight = 0;
  lcd->fillScreen(WHITE);
  lcd->setTextSize(1);
  label_Debug.drawButton();

  lastReadCount_DebugArea = NtempReads-1;
  rowIdx_DebugArea = 0;
  updateDebugScreen();

  btn_DebugDone.drawButton();
  screenButtons->registerButton(btn_DebugDone, btnPress_DebugDone);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Button press handlers for the Debug screen.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle press of Done button in Debug screen. We switch to Special screen.
/////////////////////////////////////////////////////////////////////////////////////////////
static void btnPress_DebugDone(Button_TT& btn) {
  currentScreen = SCREEN_SPECIAL;
  drawSpecialScreen();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Handle the Debug screen when it is shown.  This is called from loop().
/////////////////////////////////////////////////////////////////////////////////////////////
static void loopDebugScreen() {
  updateDebugScreen();
  // Don't turn off backlight in debug mode.
  MSsinceLastTouchBeforeBacklight = 0;
}

// *************************************************************************************** //
// Touch screen processing.
// *************************************************************************************** //

/////////////////////////////////////////////////////////////////////////////////////////////
// Master button press/release processing function. On press, start playing a tone, and on
// release, end the tone.
/////////////////////////////////////////////////////////////////////////////////////////////
static void buttonPressRelease(bool press) {
  sound->setPWM(BEEPER_PIN, TS_TONE_FREQ, press ? TS_TONE_DUTY : 0);
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
        digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_OFF);
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
    if (digitalRead(LCD_BACKLIGHT_LED) == LCD_BACKLIGHT_OFF)
      digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
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
      writeNonvolatileSettingsIfChanged(userSettings);
      //monitor.printf("Wrote settings to non-volatile memory\n");
      // Each time we transition to a new mode, we clear the run timer.
      if (userSettings.SmartVentMode != activeSettings.SmartVentMode)
        RunTimeMS = 0;
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

  // Initialize SmartVent relay and SmartVent ON/OFF and set ArmState state.
  monitor.printf("Init: SmartVent\n");
  pinMode(SMARTVENT_RELAY, OUTPUT);
  digitalWrite(SMARTVENT_RELAY, SMARTVENT_OFF);
  smartVentOn = false;
  setArmState(ARM_OFF);

  // Initialize for reading temperatures. This also initializes the ADC.
  monitor.printf("Init: Temperature\n");
  initReadTemperature(PIN_ADC_CALIB, PIN_PWM_CALIB, PIN_AREF_OUT, CFG_ADC_MULT_SAMP_AVG, wdt_reset);

  // Initialize timers.
  monitor.printf("Init: Timers\n");
  // Initialize timer for next read of temperatures.
  MSsinceLastReadOfTemperatures = 0;
  MSatLastTemperatureReadTimerUpdate = millis();
  // Initialize action-on-new-settings timers.
  MSsinceLastTouchBeforeUserSettingsActivated = USER_ACTIVITY_DELAY_MS;
  MSatLastActionTimerUpdate = millis();
  // Initialize SmartVent runtime timer.
  RunTimeMS = 0;
  MSatLastRunTimerUpdate = millis();

  // Create PWM object for sound from beeper.
  monitor.printf("Init: Sound\n");
  sound = new SAMD_PWM(BEEPER_PIN, TS_TONE_FREQ, 0);

  // Create LCD object, initialize its backlight and timers, and initialize actual displayed data.
  monitor.printf("Init: Display\n");
  lcd = new Adafruit_ILI9341(LCD_CS, LCD_DC);
  pinMode(LCD_BACKLIGHT_LED, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_OFF);
  lastNoTouchTime = millis();
  MSsinceLastTouchBeforeBacklight = LCD_BACKLIGHT_AUTO_OFF_MS * 1000;
  MSatLastBacklightTimerUpdate = millis();
  lcd->begin();
  lcd->setRotation(2);   // portrait mode
  lcd->setTextColor(BLUE);
  lcd->setTextSize(1);
  lcd->setTextWrap(false);

  // Create touchscreen object and initialize it.
  monitor.printf("Init: Touchscreen\n");
  touch = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
  touch->setRotation(lcd->getRotation());
  touch->setThresholds(Z_THRESHOLD/3);
  touch->begin();

  // Create and initialize touchscreen-LCD object.
  monitor.printf("Init: Touchscreen-Display\n");
  ts_display = new(TS_Display);
  ts_display->begin(touch, lcd);

  // Read settings from flash-based EEPROM into activeSettings, then copy them to userSettings.
  // Initialize touchscreen calibration parameter defaults from current settings in ts_display.
  monitor.printf("Init: Nonvolatile\n");
  ts_display->getTS_calibration(&settingDefaults.TS_LR_X, &settingDefaults.TS_LR_Y,
    &settingDefaults.TS_UL_X, &settingDefaults.TS_UL_Y);
  readNonvolatileSettings(activeSettings, settingDefaults);
  userSettings = activeSettings;
  ts_display->setTS_calibration(userSettings.TS_LR_X, userSettings.TS_LR_Y,
    userSettings.TS_UL_X, userSettings.TS_UL_Y);
  updateArmState();


  // Create button collection object to manage the buttons of the currently displayed screen,
  // and initialize screen buttons for all screens.
  monitor.printf("Init: Buttons\n");
  screenButtons = new Button_TT_collection;
  initButtons_MainScreen();
  initButtons_SettingsScreen();
  initButtons_AdvancedScreen();
  initButtons_CleaningScreen();
  initButtons_SpecialScreen();
  initButtons_CalibrationScreen();
  initButtons_DebugScreen();
  // Register master button press/release processing function.
  screenButtons->registerMasterProcessFunc(buttonPressRelease);

  // Show screen indicated by TEST_MODE.
  monitor.printf("Init: ShowScreen: %d\n", TEST_MODE);
  #if TEST_MODE == 3  // Touchscreen testing.
  lcd->fillScreen(WHITE);
  digitalWrite(LCD_BACKLIGHT_LED, LCD_BACKLIGHT_ON);
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
    break;
  case SCREEN_DEBUG:
    loopDebugScreen();
    break;
  }

  #endif // TEST_MODE

  // Finally, reset the watchdog timer.
  wdt_reset();
}

// *************************************************************************************** //
// End.
// *************************************************************************************** //
