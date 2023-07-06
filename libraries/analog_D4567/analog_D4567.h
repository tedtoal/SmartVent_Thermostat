/*
  This .h/.cpp file pair are a copy of the wiring_analog.h/.cpp file pair for
  the SAMD21G microprocessor (Nano 33 IoT microcomputer), with the following
  changes:
    1. The original analogRead() function doesn't accept use of D4/D5/D6/D7
      as analog inputs, even though they can be used that way. Instead, it maps
      those to A4/A5/A6/A7. This code removes that mapping and allows those pins
      to be used as analog inputs.
    2. The original analogRead() function would disable the DAC, which may not
      be what is wanted if the DAC is being used to drive the ADC pin for
      calibration, so this implements an argument to leave the DAC enabled.
    3. The original analogWrite() function had some problems with regard to
      using a timer for pulse width modulation:
      - it used a fixed value of 0xFFFF for the waveform period with a TCC timer.
      - on the first call to analogWrite() for a given timer, the timer is
        initialized and the CC register is written, but not the CCB register.
        On subsequent calls the timer is not initialized and the CCB register
        is written. I think initialization should be separate from update, and
        the user should be able to re-initialize a timer whenever he wants.
      To solve these issues, two functions were added:
        analogStartPWM_TCC_D4567: initialize a timer for PWM, including period
        analogUpdatePWM_TCC_D4567: write an update to the PWM, including period
      The functions return true if successful, false if argument error.
    4. The resolution for the PWM was shared with that for the ADC, and could be
      changed at any time. This separates the write resolutions, adding functions
      analogWriteResolution_PWM_D4567() and analogWriteResolution_ADC_D4567().
    5. Every function implemented in this module has the same name as the
      original, with "_D4567" appended, e.g. analogRead becomes analogRead_D4567.

  When using this, use ALL the functions herein.  It would be a good idea to
  call the original AND these functions for setting read/write resolution and
  analog reference.
*/
#include <Arduino.h>

extern void analogReadResolution_D4567(int res);
extern void analogWriteResolution_D4567(int res);
extern void analogWriteResolution_PWM_D4567(int res);
extern void analogWriteResolution_ADC_D4567(int res);
extern void analogReference_D4567(eAnalogReference mode);
extern int analogRead_D4567(pin_size_t pin, bool disableDAC=true);
extern void analogWrite_D4567(pin_size_t pin, int value);
extern bool analogStartPWM_TCC_D4567(pin_size_t pin, int value, int PWMperiod);
extern bool analogUpdatePWM_TCC_D4567(pin_size_t pin, int value, int PWMperiod);
