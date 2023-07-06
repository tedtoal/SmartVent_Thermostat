/*
  nonvolatileSettings.cpp
*/
// It appears (page 29 of Atmel SAM D21E / SAM D21G / SAM D21J data sheet) that the page size is 64,
// and 4 pages must be erased at one time, giving an effective page size of 4*64 = 256.  This seems
// to be confirmed by the fact that FlashStorage_SAMD.hpp aligns the PPCAT variable to 256 bytes.
#define EEPROM_EMULATION_SIZE     (4 * 64)

// Use 0-2. Larger for more debugging messages
#define FLASH_DEBUG       0

// To be included only in one file to avoid `Multiple Definitions` Linker Error
#include <FlashStorage_SAMD.h>

#include "nonvolatileSettings.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables.
/////////////////////////////////////////////////////////////////////////////////////////////

// Signature used at start of a flash memory block to mark the block as containing
// valid data written by the application.
const int WRITTEN_SIGNATURE = 0xBEEFDEED;

// The currently active settings (initialized from flash-based EEPROM).
nonvolatileSettings activeSettings;

// The current settings seen by the user, held here until
// USER_ACTIVITY_DELAY_SECONDS has elapsed with no screen touches,
// at which time this is copied to "activeSettings".
nonvolatileSettings userSettings;

// The default settings, used to initialize empty flash-based EEPROM.
const nonvolatileSettings settingDefaults = {
  MODE_OFF,   // SmartVentMode: SmartVent mode
  78,         // TempSetpointOn: Indoor temperature setpoint in °F for SmartVent to turn on.
  7,          // DeltaTempForOn: °F indoor must exceed outdoor temperature to turn on SmartVent.
  3,          // HysteresisWidth: Hysteresis °F, band around TempSetpointOn and DeltaTempForOn to turn on/off.
  4,          // MaxRunTimeHours: Run time limit in hours in AUTO mode, 0 = none
  1,          // DeltaArmTemp: °F outdoor must exceed indoor temperature to rearm SmartVent for the next day.
  0,          // IndoorOffsetF: Amount to add to measured indoor temperature in °F to get temperature to display.
  0           // OutdoorOffsetF: Amount to add to measured outdoor temperature in °F to get temperature to display.
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Read non-volatile settings from flash memory into settings.
/////////////////////////////////////////////////////////////////////////////////////////////
void readNonvolatileSettings(nonvolatileSettings& settings, const nonvolatileSettings& defaults) {
  // Initialize flash-based EEPROM to only commit data when we call the commit function.
  EEPROM.setCommitASAP(false);
  // Check signature at address 0
  int signature;
  uint16_t storedAddress = 0;
  EEPROM.get(storedAddress, signature);
  // If flash-based EEPROM is empty, write WRITTEN_SIGNATURE and defaults data to it.
  if (signature != WRITTEN_SIGNATURE) {
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
    EEPROM.put(storedAddress + sizeof(signature), defaults);
    EEPROM.commit();
  }

  // Read settings data from flash-based EEPROM.
  EEPROM.get(storedAddress + sizeof(signature), settings);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Write non-volatile settings to flash memory IF IT HAS CHANGED.
/////////////////////////////////////////////////////////////////////////////////////////////
bool writeNonvolatileSettingsIfChanged(nonvolatileSettings& settings) {
  int signature;
  uint16_t storedAddress = 0;
  nonvolatileSettings tmp;
  EEPROM.get(storedAddress + sizeof(signature), tmp);
  if (memcmp(&settings, &tmp, sizeof(nonvolatileSettings)) == 0)
    return(false);
  EEPROM.put(storedAddress + sizeof(signature), settings);
  EEPROM.commit();
  return(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
