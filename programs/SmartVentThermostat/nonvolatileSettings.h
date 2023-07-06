/*
  nonvolatileSettings.h

  Define constants, structs, variables, and functions for managing SmartVent thermostat
  non-volatile settings, which are stored in the Nano 33 IoT flash memory.
*/
#include <Arduino.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// Constants.
/////////////////////////////////////////////////////////////////////////////////////////////

// Minimum and maximum temperature setpoints in degrees F.
#define MIN_TEMP_SETPOINT 50
#define MAX_TEMP_SETPOINT 99

// Minimum and maximum SmartVent-On indoor/outdoor temperature differential in degrees F.
#define MIN_TEMP_DIFFERENTIAL 2
#define MAX_TEMP_DIFFERENTIAL 20

// Minimum and maximum SmartVent-off temperature hysteresis in degrees F.
// The hysteresis is subtracted from both the indoor setpoint and the indoor/outdoor
// differential to get the values used to decide when to turn SmartVent off.
#define MIN_TEMP_HYSTERESIS 1
#define MAX_TEMP_HYSTERESIS 9

// Maximum SmartVent run time limit in hours.
#define MAX_RUN_TIME_IN_HOURS 9

// String to show on LCD when max run time is 0.
#define MAX_RUN_TIME_0 "--"

// Maximum SmartVent delta arm temperature in degrees F. After SmartVent runs for the
// maximum run time, it is turned off and disarmed from turning on again UNTIL the
// outdoor temperature exceeds the indoor temperature by the delta arm temperature.
// This ensures that SmartVent runs only once per day for the specified maximum run
// time limit.
#define MAX_DELTA_ARM_TEMP 20

// String to show on LCD when max run time is 0.
#define MAX_RUN_TIME_0 "--"

// Maximum temperature calibration adjustment in degrees F. Minimum is negative of this.
#define MAX_TEMP_CALIB_DELTA 9

/////////////////////////////////////////////////////////////////////////////////////////////
// Structs and enums.
/////////////////////////////////////////////////////////////////////////////////////////////

// SmartVent mode.
typedef enum _eSmartVentMode {
  MODE_OFF,
  MODE_ON,
  MODE_AUTO
} eSmartVentMode;

// Structure containing non-volatile data to be stored in flash memory (with copy in regular memory).
struct nonvolatileSettings {
  eSmartVentMode SmartVentMode; // SmartVent mode
  uint8_t TempSetpointOn;       // Indoor temperature setpoint in °F for SmartVent to turn on.
  uint8_t DeltaTempForOn;       // Indoor temperature must exceed outdoor by this to turn on SmartVent.
  uint8_t HysteresisWidth;      // Hysteresis °F, band around TempSetpointOn and DeltaTempForOn to turn on/off.
  uint8_t MaxRunTimeHours;      // Run time limit in hours (AUTO or ON mode, AUTO is cumulative), 0 = none
  uint8_t DeltaArmTemp;         // Outdoor temperature must exceed indoor by this to rearm SmartVent for next day.
  // Note: Following two values are NOT incorporated into temperatures read by thermistorAndTemperature library.
  // Instead, they are only used to adjust curIndoorTemperature and curOutdoorTemperature before they are used in
  // SmartVentThermostat.ino.
  int8_t IndoorOffsetF;         // Amount to add to measured indoor temperature in °F to get temperature to display.
  int8_t OutdoorOffsetF;        // Amount to add to measured outdoor temperature in °F to get temperature to display.
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables.
/////////////////////////////////////////////////////////////////////////////////////////////

// The currently active settings (initialized from flash-based EEPROM and stored in EEPROM
// each time the data changes via copy from userSettings).
extern nonvolatileSettings activeSettings;

// The current settings seen by the user, held here until
// USER_ACTIVITY_DELAY_SECONDS has elapsed with no screen touches,
// at which time this is copied to "activeSettings" and the latter
// is then stored in EEPROM.
extern nonvolatileSettings userSettings;

// The default settings, used to initialize empty flash-based EEPROM.
extern const nonvolatileSettings settingDefaults;

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Read non-volatile settings from flash memory into settings.  If flash memory has not yet
// been initialized, initialize it with defaults.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void readNonvolatileSettings(nonvolatileSettings& settings,
  const nonvolatileSettings& defaults);

/////////////////////////////////////////////////////////////////////////////////////////////
// Write non-volatile settings to flash memory IF IT HAS CHANGED. Return true if it changed
// and was written, else false.
/////////////////////////////////////////////////////////////////////////////////////////////
extern boolean writeNonvolatileSettingsIfChanged(nonvolatileSettings& settings);

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
