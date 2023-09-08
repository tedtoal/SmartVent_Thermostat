/*
  temperature.h - Define constants structs, variables, and
  functions to support the reading of indoor and outdoor temperatures from
  indoor and outdoor thermistors.
  Created by Ted Toal, 17-Aug-2023
  Released into the public domain.


  Software License Agreement (BSD License)

  Copyright (c) 2023 Ted Toal
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holders nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*/
#ifndef temperature_h
#define temperature_h

#include <Arduino.h>

/*
  Thermistor calculation details.

  The Steinhart–Hart equation is a model of the resistance of a semiconductor at
  different temperatures. The equation is:

      1/T = A + B ln R + C(ln ⁡R)^3

    where:

      T is the temperature (in Kelvins)
      R is the resistance at T (in ohms)
      A, B, and C are the Steinhart–Hart coefficients, which vary depending on the
        type and model of thermistor and the temperature range of interest.

  File thermistors.py contains Python functions for working with thermistors.
  These functions are:

    ***************************************************************************
    Round x to 'digits' significant digits and return it:
    ***************************************************************************
    signif(x, digits=6)

    ***************************************************************************
    To convert between Kelvin, Celsius, and Fahrenheit (arguments can be either
    numeric or a list of numbers and return value is correspondingly a number or
    a list):
    ***************************************************************************
    degFtoC(degF): Fahrenheit to Celsius
    degCtoF(degC): Celsius to Fahrenheit
    degCtoK(degC): Celsius to Kelvin
    degKtoC(degK): Kelvin to Celsius

    ***************************************************************************
    Compute temperature T from thermistor resistances (rt, in ohms) and thermistor
    coefficients A/B/C. The rt argument can be a number or a list of numbers, and
    the return value is a number or list, respectively. Print the returned values
    if printVals=True.
    ***************************************************************************
    T_C(rt, A, B, C): Return temperature in Celsius
    T_F(rt, A, B, C): Return temperature in Fahrenheit

    ***************************************************************************
    If the thermistor is in series with a resistor of resistance r and the ADC measures
    the voltage at the junction between the two, then the thermistor resistance can be
    calculated with Ohm's law:  R = r*(1/ADC-1)
    Given Vadc = ADC value(s) of the voltage at the connection between an unknown
    resistance Rt (the thermistor) and a series resistance rs, with a maximum ADC
    reading of Vmax, this returns thermistor resistance Rt. Vout can be a number or
    list of numbers, and the return value is a number or list, respectively, of
    thermistor resistance(s).
    This assumes that the ADC reference voltage is at the other side of the
    thermistor and that the other side of the series resistor is grounded.
    ***************************************************************************
    Rt_fromADCvalues(Vout, Vmax, rs)

    ***************************************************************************
    Likewise, if the thermistor resistance is known along with the series resistance,
    nte expected ADC reading can be computed as: ADC = r/(r+R)
    Given a thermistor resistance rt, a series resistance rs, and a maximum ADC reading
    of Vmax, this computes the ADC reading at the junction between the resistors.
    The rt argument can be a number or list, and the return value is a number or
    list of numbers, respectively, of ADC junction readings.
    ***************************************************************************
    Vout_ADC(rt, rs, Vmax)

    ***************************************************************************
    It is also possible to solve for R given T and A/B/C, which might be useful at times.
    Given temperature TC in Celsius and thermistor A/B/C coefficients, compute
    expected thermistor resistance. The TC argument can be a number or a list of
    numbers, and the return value is a number or list, respectively, of thermistor
    resistance(s). Print returned values if printVals=True.
    ***************************************************************************
    Rt_fromTC_ABC(TC, A, B, C)

    ***************************************************************************
    Given resistances R1, R2, and R3 at temperatures T1, T2, and T3, then A, B, and
    C can be computed.
    Given TC[0:2] with three temperatures (in Celsius) and Rt[0:2] with 3 thermistor
    resistances at those temperatures (in ohms), compute thermistor coefficients
    A/B/C, returned in a list. Print the three values if printVals=True.
    ***************************************************************************
    ABCatRT(TC, Rt, printVals=True)

  ***************************************************************************
  ***************************************************************************
  ***************************************************************************
  ***************************************************************************

  Using the Python functions in thermistors.py, I computed the A/B/C
  coefficients for a number of different thermistors.

  A data sheet shows that a 10K type 3 thermistor has these resistances at
  these temperatures:
        0°C     29490
        25°C    10000
        50°C    3893
  Find coefficients:
    ABCatRT([0, 25, 50), [29490, 10000, 3893])   # 10K type 3
      A = 0.001029, B = 0.0002391, C = 1.566e-07
  Confirming:
    T_C([29490, 10000, 3893], A = 0.001029, B = 0.0002391, C = 1.566e-07)
      0.03792 25.04 50.05  # Pretty close
  And checking R values:
    Rt_fromTC_ABC([0, 25, 50], A = 0.001029, B = 0.0002391, C = 1.566e-07)
      29542 10017 3899 # Close enough?

  And a 10K type 2 has these resistances:
        0°C     32650
        25°C    10000
        50°C    3602

    ABCatRT([0, 25, 50], [32650, 10000, 3602])   # 10K type 2
      A = 0.001127, B = 0.0002344, C = 8.675e-08
    T_C([32650, 10000, 3602], A = 0.001127, B = 0.0002344, C = 8.675e-08)
      0.02465 25.03 50.04  # Close enough
    Rt_fromTC_ABC([0, 25, 50], A = 0.001127, B = 0.0002344, C = 8.675e-08)
      32691 10013 3607 # Close enough

  Based on testing, my outdoor thermistor seems to be a type 2.

  A web site gives a Chinese data sheet for the Arduino kit's thermistor.  It has R at 25°C = 10K,
  and it has B=3950, where B = ln(Ra/Rb)*(Ta*Tb)/(Ta-Tb), which gives R at 50°C = 3588..
  It also gives a table of resistances at different temperatures, but three different columns are
  given and I can't read Chinese so I don't know what the difference is between the columns.  But,
  at 0°C the columns show 31.426, 32.116, and 32.817.  At 25°C they show 9.900, 10.000, and 10.100,
  suggesting that the middle column is the average.  At 50°C they show 3.515, 3.588, and 3.661.  My
  thermistor measures about 10.2K resistance at about 24°C, while the B equation gives 10.456, so
  it is close, but plugging in 10.2K gives 24.6°C, so it seems about right.

  If we compute A/B/C for each column of their table:
    ABCatRT([0, 25, 50], [31426, 9900, 3515])   # Arduino thermistor, Chinese table first column
      A = 0.001375, B = 0.000194, C = 2.499e-07
    ABCatRT([0, 25, 50], [32116, 10000, 3588])   # Arduino thermistor, Chinese table second column
      A = 0.001237, B = 0.000216, C = 1.634e-07
    ABCatRT([0, 25, 50], [32817, 10100, 3661])   # Arduino thermistor, Chinese table third column
      A = 0.0011, B = 0.0002377, C = 7.874e-08

  Standard Arduino thermistor sample code values for A/B/C:  A = 0.001009249522, B = 0.0002378405444, C = 2.019202697e-07
  Those lie roughly in the same ballpark as above, but C seems off.

  My Arduino starter kit thermistor wasn't giving an accurate reading, so I bought two different
  thermistors. One is an MF52B 3950 100K NTC thermistor.  Its resistances are:
        0°C     321K
        25°C    100K
        50°C    35.8K
    ABCatRT([0, 25, 50], [321000, 100000, 35800]) # MF52B 3950 100K NTC
      A = 0.0008162, B = 0.0002019, C = 1.395e-07

  The second is an M52B 3435 10K NTC thermistor.  As best I can tell, it has these resistances:
        0°C     31770
        25°C    10000
        50°C    3592
    ABCatRT([0, 25, 50], [31770, 10000, 3592])  # M52B 3435 10K NTC
      A = 0.001283, B = 0.0002079, C = 2.005e-07

  I measured the M52B 3435 10K NTC thermistor resistance at room temperature, in ice water, and hot water:
        36°F    26000
        70°F    11800
        115°F   4740

    ABCatRT(degFtoC([36, 70, 115]), [26000, 11800, 4740]) # M52B 3435 10K NTC my measurements
      A = 0.0007609, B = 0.0002752, C = 6.933e-08
    T_F(11800, A = 0.0007609, B = 0.0002752, C = 6.933e-08)
    gives: 70.02

  I bought another thermistor:
    EPCOS p/n: B57862S103F, NTC Thermistor 10K Ohms 1% 3988K 60mW Mini Bead with Wires:
        0°C     32650
        25°C    10000
        50°C    3603
    ABCatRT([0, 25, 50], [32650, 10000, 3603])  # EPCOS p/n: B57862S103F, NTC 10K Ohms 1% 3988K 60mW
      A = 0.001125, B = 0.0002347, C = 8.563e-08
*/

/////////////////////////////////////////////////////////////////////////////////////////////
// Constants.
/////////////////////////////////////////////////////////////////////////////////////////////

// Define this as 0 to use the usual Arduino Nano 33 IoT analog/digital
// converter functions, and define it as 1 to instead use those in
// analogRead_SAMD_TT.h/cpp. The latter allows for using D4, D5, D6, and D7 as
// analog inputs, and fixes some errors in the module. This is REQUIRED when
// calibrating the ADC with module calibSAMD_ADC_withPWM.
// If this is set to 0, you must provide code that is currently located in
// calibSAMD_ADC_withPWM.cpp:
//    1. Define PIN_AREF_OUT and set its pinMode and initially turn it off.
//    2. Select external voltage reference for the ADC.
//    3. Select 12-bit ADC resolution.
//    4. Load the ADC with gain and offset error correction values if desired.
//    5. Configure multiple sampling and averaging, if desired.
// The actual pin numbers used by this module are set in the .cpp file definitions of
// IndoorThermistor and OutdoorThermistor, as well as in the constants defined in
// calibSAMD_ADC_withPWM.h.
#define USE_ANALOG_SAMD 1

// Number of temperature readings to buffer and compute running average, for reduction of
// jitter in thermistor readings.  Note: we also enable the ADC converter to internally
// take a number of samples and average them, each time we tell it to do a conversion.
#define NUM_TEMPS_RUNNING_AVG 30

// Hysteresis used by roundTemperature. Refer to its comments for an explanation.
// Separate values are used for C and F degrees, but generally it makes sense for the
// Fahrenheit value to be 9/5 times the Celsius value. A value of 0 turns off hysteresis.
#define TEMP_HYST_C 0.125
#define TEMP_HYST_F 0.25

// Number of milliseconds to delay after turning on AREF before reading ADC. Outside sensor
// may have considerable capacitance. On mine a scope shows 1 ms is too little.
#define AREF_STABLE_DELAY 3

// Force indoor or outdoor temperature to this value in °C, for debugging. Set these to 9999
// to not do this and use the measured temperature. Note: 30°C = 86°F.
#define FORCE_INDOOR_TEMP   9999
#define FORCE_OUTDOOR_TEMP  9999

// Offset in °C to apply to FORCE_INDOOR_TEMP or FORCE_OUTDOOR_TEMP during initialization of
// temperatures in the temperature buffer. Used only when they are not 9999. Note: 10°C = 18°F.
#define DEBUG_TEMP_OFFSET   (-10)

/////////////////////////////////////////////////////////////////////////////////////////////
// Structs.
/////////////////////////////////////////////////////////////////////////////////////////////

// Structure for holding thermistor parameters.
struct thermistor {
  pin_size_t inputPin;
  uint16_t seriesResistor;
  float A;
  float B;
  float C;
};

// Structure for holding temperature computation results. For explanation of goingUpC and
// goingUpF, see roundTemperature(). ADC is the ADC reading and Rthermistor is the computed
// thermistor resistance (both for debugging).
struct temperature {
  float Tc;
  int16_t Tc_int16;
  float Tf;
  int16_t Tf_int16;
  bool goingUpC;
  bool goingUpF;
  uint16_t ADCvalue;
  uint16_t Rthermistor;
};

// Structure for holding circular temperature buffer of Celsius temperatures for computing
// running average.
struct temperatureBuf {
  float Tc[NUM_TEMPS_RUNNING_AVG];
  uint8_t idxLatest;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables.
/////////////////////////////////////////////////////////////////////////////////////////////

// Thermistor parameters.
// Note: Series resistors were measured with an ohmmeter
extern const thermistor IndoorThermistor;
extern const thermistor OutdoorThermistor;

// Buffers for buffering last several temperatures that were read, for computing running
// average.
extern temperatureBuf IndoorTempBuf;
extern temperatureBuf OutdoorTempBuf;

// Current indoor and outdoor temperatures (result of running averages).
extern temperature curIndoorTemperature;
extern temperature curOutdoorTemperature;

// Number of times indoor and outdoor temperatures have been read, for debugging.
extern uint16_t NtempReads;

// Values of computed thermistor resistances on last indoor and outdoor temperature reads,
// for debugging.
extern uint16_t RlastIndoorTempRead;
extern uint16_t RlastOutdoorTempRead;

// Values of ADC on last indoor and outdoor temperature reads, for debugging.
extern uint16_t ADClastIndoorTempRead;
extern uint16_t ADClastOutdoorTempRead;

// Values of computed temperatures on last indoor and outdoor temperature reads,
// for debugging.
extern float TlastIndoorTempRead;
extern float TlastOutdoorTempRead;

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions.
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize for reading indoor and outdoor temperatures. Currently this also initializes
// the ADC converter, which is currently used in this project only for reading thermistors.
// Arguments:
//  pinADC_CALIB: pin number of analog input connected to calibration capacitor.
//  pinPWM_CALIB: pin number of TCC output connected to calibration resistor.
//  pinAREF_OUT: pin number of digital output connected to AREF.
//  cfgADCmultSampAvg: multi-sample averaging, 0 = disabled, n = 2^n samples are averaged.
//  periodicallyCall: pointer to function to call during periods of long activity
//    (e.g. watchdog timer reset function), nullptr for none.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void initReadTemperature(int pinADC_CALIB, int pinPWM_CALIB, int pinAREF_OUT,
  uint8_t cfgADCmultSampAvg, void (*periodicallyCall)());

/////////////////////////////////////////////////////////////////////////////////////////////
// Convert degrees C to degrees F, and degrees C to degrees K, and vice-versa.
/////////////////////////////////////////////////////////////////////////////////////////////
inline float degCtoF(float TC) { return(TC*9.0/5.0+32.0); }
inline float degFtoC(float TF) { return((TF-32.0)*5.0/9.0); }
inline float degCtoK(float TC) { return(TC+273.15); }
inline float degKtoC(float TK) { return(TK-273.15); }

/////////////////////////////////////////////////////////////////////////////////////////////
// Compute rounded version of a floating point temperature value (rounded to an integer).
// Temp is the temperature to be rounded, goingUp is true if the last time the rounded
// value changed it INCREASED (false for DECREASED), and isCelsius is true if Temp is in
// Celsius degrees and false if Fahrenheit degrees.
//
// This algorithm introduces hysteresis to rounding, to reduce jitter when the temperature
// hovers near the boundary between two integers. This is accomplished by using knowledge
// of what direction the temperature has been running, which is why the goingUp argument is
// required. In normal rounding, if the fraction part is < 0.5 we would round down, and if
// >= 0.5 we round up. With hysteresis, the threshold of 0.5 is changed by either adding or
// subtracting one half of a hysteresis value (TEMP_HYST_C or TEMP_HYST_F depending on
// isCelsius, both defined above), subtracting when goingUp is true and adding when false.
//
// As an example, if the hysteresis is 0.25, then half of that is 0.125. When goingUp
// is true, the threshold for rounding is 0.5 - 0.125 = 0.375. Say the rounded temperature
// is 70. Then it will increase to 71 at 70.375 and it will decrease to 69 at 69.374.
// Suppose the reading is 69.374, so this is rounded to 69, and goingUp now becomes false.
// The new threshold is now 0.5 + 0.125 = 0.625. The rounded temperature will now
// decrease one degree further to 68 at 68.624, but it will not increase BACK to 70
// until the temperature reaches 69.625. Since it was just 69.374 when it went down to
// 69, if jitter occurs and it reads a bit more than that, say 69.54, this is not enough
// to exceed the threshold at 69.625, so the temperature remains 69 rather than jittering
// around between 69 and 70.
/////////////////////////////////////////////////////////////////////////////////////////////
extern int16_t roundTemperature(float Temp, bool goingUp, bool isCelsius);

/////////////////////////////////////////////////////////////////////////////////////////////
// Read temperature from the specified thermistor and return results in Temp.  Results
// include Temp.Rthermistor, the measured resistance of the thermistor.
//
// IMPORTANT: On call, Temp must contain valid "goingUpC" and "goingUpF" values that can be
// used by roundTemperature() to do its rounding. This retains the same values for goingUpC
// and goingUpF in the returned temperature. If turnAREFoff is set FALSE, the ADC reference
// voltage output pin is left on upon exit. If called again, it will see that it is on and
// will not turn it on and execute the AREF_STABLE_DELAY delay, saving time. This can be
// used when reading indoor and outdoor temperatures back-to-back.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void readTemperature(const thermistor& Thermistor, temperature& Temp, bool turnAREFoff=true);

/////////////////////////////////////////////////////////////////////////////////////////////
// Nano 33 IoT has problems with stable ADC, it jitters a lot. This function updates TempBuf
// by reading the current temperature and adding it to TempBuf (replacing the oldest in the
// buffer), then recomputes the running average of the buffer values and returns the average
// in Temp. A running average is only computed for the Tc member of the temperature struct,
// and Tc_uint16, Tf, and Tf_uint16 are computed directly from the average Tc. The Rthermistor
// member of Temp is set to the last measured Rthermistor value, no running average is
// computed for it. turnAREFoff is passed to readTemperature().
//
// IMPORTANT: On call to this function, Temp must contain the PREVIOUS TEMPERATURE from the
// same thermistor, so its "goingUpC" and "goingUpF" values can be used to determine by
// roundTemperature() to do its rounding. This UPDATES the values for goingUpC and goingUpF
// in the returned temperature, according to the change observed in Tc_int16 and Tf_int16
// from the input temperatures to the output temperatures.
//
// This returns the new temperature that was read (in Celsius) and added to the buffer.
// On return, Temp contains the new average temperatures.
/////////////////////////////////////////////////////////////////////////////////////////////
extern float readTemperatureRunningAverage(const thermistor& Thermistor,
  temperatureBuf& TempBuf, temperature& Temp, bool turnAREFoff=true);

/////////////////////////////////////////////////////////////////////////////////////////////
// Read indoor and outdoor temperatures, updating the running averages and storing them in
// curIndoorTemperature and curOutdoorTemperature. NtempReads is incremented,
// ADClastIndoorTempRead and ADClastOutdoorTempRead are set to the most recent ADC value,
// RlastIndoorTempRead and RlastOutdoorTempRead are set to the most recent computed
// Rthermistor value, and TlastIndoorTempRead and TlastOutdoorTempRead are set to the most
// recent computed temperature (Celsius).
/////////////////////////////////////////////////////////////////////////////////////////////
extern void readCurrentTemperatures(void);

/////////////////////////////////////////////////////////////////////////////////////////////
// Display a temperature on the Arduino IDE serial monitor, with a prefix description string.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void showTemperature(const temperature Temp, char* Desc);

/////////////////////////////////////////////////////////////////////////////////////////////
// Read indoor and outdoor temperatures using readCurrentTemperatures() and write them to the
// serial monitor.
/////////////////////////////////////////////////////////////////////////////////////////////
extern void readAndShowCurrentTemperatures(void);

#endif // temperature_h
