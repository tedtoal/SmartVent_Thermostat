//******************************************************************************
// calibADC_gain_offset_withPWM
// Author: Ted Toal
// Date: 21-May-2023
//
// The SAMD21G Nano 33 IoT ADC performance is bad, even after using the
// calibration procedure in program CorrectADCResponse.ino. That program assumes
// that 0 and 3.3V are the two voltage-ends of the ADC, but the ADC seems to have
// problems at the ends.
//
// This module defines function fixADC_GainOffset() that computes ADC gain and
// offset calibration parameters using the ADC and a TCC timer with pulse width
// modulation (PWM).
//
// This code is customized for use in a SAMD21 microcomputer, such as in the
// Arduino Nano 33 IoT. It won't work for non-SAMD21 systems, it would require
// changes.
//
// This configures the ADC for 12-bit operation, and assumes the user uses that
// resolution with the ADC.
//
// This code uses the PWM output fed into a resistor and capacitor in series,
// to generate reliable voltages between GND and 3.3V.
//
// This requires use of the ADC and a TCC timer, a digital output that can be
// driven by the timer, and an analog input that can be read by the ADC. It also
// uses an external voltage reference for the ADC, and generates that voltage
// using another digital output port.
//
// Circuit wiring:
//  1. Use a 0.1 uF capacitor between GND and pin PIN_CAP.
//  2. Use a 10K ohm resistor between PIN_CAP and PIN_PWM.
//  3. Use a large (say 100 uF) capacitor between GND and PIN_AREF_OUT.
//  4. Wire PIN_AREF_OUT to the AREF (external analog reference voltage) pin.
//
// Alternatively, wire the AREF pin directly to +3.3V and don't use PIN_AREF_OUT.
//
// Using this module:
//  1. Set PIN_ constants below to the pins you wish to use for PIN_CAP, PIN_PWM,
//      and PIN_AREF_OUT (optional), and check the other constants for values
//      that you may want to change.
//  2. #include this .h file in your program
//  3. Call fixADC_GainOffset() from setup() before using the ADC.
//  4. Wire your system as shown above.
//  5. Compile your program and download it into a SAMD21-based microcomputer.
//  6. When fixADC_GainOffset(), it will compute and load gain and offset
//      corrections into the ADC. ADC reads should be more accurate.
//
// This requires use of my library module named analog_D4567, which has several
// changes for fixes to ADC and PWM problems, see the comments in the .h file.
//
// This optionally uses my library module named monitor_printf(). If you enable
// it below, fixADC_GainOffset() will print out ADC values that you can check to
// make sure the function is working properly.
//******************************************************************************

#include <Arduino.h>

// Change the pin assignments below to match your system.

// Arduino "pin" number of analog input pin to use to read the calibration
// voltage. This is actually an index into g_APinDescription[] in variant.cpp,
// it is not an actual pin number.
// This pin has an 0.1 uF capacitor tied between it and ground, and a 10K ohm
// resistor tied between it and PIN_PWM.
// NOTES ON FINDING variant.cpp and other system files:
//  Right-click on "#include <Arduino.h>" in Arduino IDE and choose "Go to
//  Definition" to open the file in the IDE. From there, open WVariant.h,
//  variant.h, and samd.h. In WVariant.h, right-click on "extern const
//  PinDescription g_APinDescription[];" and choose "Go to Definition" to open
//  variant.cpp. From samd.h, open samd21.h, and from there, open samd21g18a.h,
//  and from there open both the instance and component .h files for the
//  peripherals of interest, say adc.h and tcc.h. To open wiring_analog.c,
//  insert a call to analogWrite(), right-click and choose "Go to Definition".
//  In that file, find a call to pinPeripheral and right-click it and choose "Go
//  to Definition" to open wiring_private.c, and look at that function to see
//  what it does. In samd21g18a.h note the definitions of symbols like ADC and
//  DAC (the second definition, not the first). To find the path of an open
//  file, hover over its name in Arduino IDE open tab for that file. Also open
//  analog_D4567.cpp and compare to wiring_analog.c.
// NOTE ON MICROPROCESSOR DATASHEET: download the data sheet by going to:
//  https://www.microchip.com/en-us/product/ATsamd21g18
//  and downloading the data sheet PDF (over 1000 pages, daunting!) To use it,
//  study table 7-1 and note how g_APinDescription[] in variant.cpp uses its
//  info. Then, for each peripheral of interest, in the Table of Contents at the
//  top, find the chapter for that peripheral. You can read the details of the
//  peripheral, or go to the "Register Summary" for it, click on registers and
//  read their description. The .h files mentioned above define all registers of
//  all peripherals in a clean easy-to-use manner. E.g., ADC CTRLB register can
//  be accessed with ADC->CTRLB.reg, and the PRESCALER field in that register
//  can be accessed with ADC->CTRLB.bit.PRESCALER.
#define PIN_CAP 7

// Arduino "pin" number of digital output pin to use to generate a PWM waveform
// using a SAMD21G TCC timer. This is also an index into g_APinDescription[] in
// variant.cpp. This pin has a 10K ohm resistor tied between it and PIN_CAP.
// Note: We use a TCC timer and not a TC timer in order that we can set the
// period of the waveform. See table g_APinDescription[] in variant.cpp for
// valid pins that have a pin attribute value of PIN_ATTR_PWM and a PWM channel
// number starting with PWM0_, PWM1_, or PWM2_, indicating use of TCC timer 0,
// 1, or 2 resp. Values higher than 2 indicate use of a TC timer, which has no
// control over the period of the waveform.
#define PIN_PWM 4

// Arduino "pin" number of digital output pin to use to generate ADC reference
// voltage. This is also an index into g_APinDescription[] in variant.cpp. This
// pin has a 100 uF capacitor tied between it and ground, and the pin is also
// tied to the external AREF input. If you don't want to use this pin but
// instead want to tie the AREF input directly to 3.3V, set this to -1.
#define PIN_AREF_OUT 6

// Number of milliseconds to delay after turning on AREF before the reference
// voltage is stable. The larger the capacitor on PIN_AREF_OUT, the longer this
// should be.
#define AREF_STABLE_DELAY 5

// Number of milliseconds of delay after turning on PWM output before the
// voltage at the PWM capacitor is stable, in the worst case. This value can be
// determined with a scope and a section of code below, see comments below.
#define PWM_STABLE_DELAY 5

// Percentage of ADC range to use, e.g. if this is 10, measure the ADC value at
// 10% and 90% of reference voltage and compute gain and offset corrections.
#define PERCENT_AT_ENDS 10

// Set this to 0 to disable ADC multiple sampling and averaging, or a number X
// between 1 and 10 to average 2^X samples, e.g. 6 means average 2^6 = 64 samples.
// This is internal hardware-based averaging. It produces more stable ADC values.
#define CFG_ADC_MULT_SAMP_AVG 6

// Maximum ADC input value (minimum is 0).
#define ADC_MAX 0xFFF

// Set this to 1 to include monitor_printf.h and print out results of the
// calibration procedure to the Arduino IDE monitor output. If set to 1, you
// must call monitor_init(desiredBaudRate) from setup() BEFORE calling
// fixADC_GainOffset().
#define USE_MONITOR_PRINTF 0

//******************************************************************************
// Run a calibration algorithm on the ADC using the PWM output and TCC timer,
// compute ADC gain and offset constants, and load them into the ADC.
//
// NOTE: 12-bit ADC resolution is set and required.
//******************************************************************************
extern void fixADC_GainOffset();
