/*
  thermistorAndTemperature.cpp
*/
#include <Arduino.h>
#include <floatToString.h>
#include <monitor_printf.h>
#include "thermistorAndTemperature.h"
#if USE_ANALOG_SAMD
#include <wiring_analog_SAMD_TT.h>
#endif
#include <calibADC_gain_offset_withPWM.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// Variables.
/////////////////////////////////////////////////////////////////////////////////////////////

// Thermistor parameters.
#if USE_ANALOG_SAMD
// Note: AZ Touch MKR A4 is Nano 33 IoT A1
//const thermistor IndoorThermistor = { A1, 10000, 0.001009, 0.0002378, 2.0192e-07 }; // Frequent sample code values for Arduino kit thermistor
//const thermistor IndoorThermistor = { A1, 10000, 0.001237, 0.000216, 1.634e-07 }; // Chinese data sheet for Arduino kit thermistor
//const thermistor IndoorThermistor = { A1, 10000, 0.0008162, 0.0002019, 1.395e-07 }; // MF52B 3950 100K NTC
//const thermistor IndoorThermistor = { A1, 10000, 0.001283, 0.0002079, 2.005e-07 }; // M52B 3435 10K NTC
//const thermistor IndoorThermistor = { A1, 10000, 0.0007609, 0.0002752, 6.933e-08 }; // M52B 3435 10K NTC, my calibration measurements
const thermistor IndoorThermistor = { A1, 10000, 0.001125, 0.0002347, 8.563e-08 }; // EPCOS B57862S103F, NTC 10K Ohms 1% 3988K 60mW
#else
#error "Indoor thermistor's current analog input needs to be revised.
#endif

// Note: A6 refers to Nano 33 IoT A6, not AZ Touch MKR A6.
const thermistor OutdoorThermistor = { A6, 10000, 0.001127, 0.0002344, 8.675e-08 };  // 10K type 2
//const thermistor OutdoorThermistor = { A6, 10000, 0.001029, 0.0002391, 1.566e-07 };// 10K type 3

// Buffers for buffering last several temperatures that were read, for computing running average.
temperatureBuf IndoorTempBuf;
temperatureBuf OutdoorTempBuf;

// Current indoor and outdoor temperatures (result of running averages).
temperature curIndoorTemperature;
temperature curOutdoorTemperature;

// Number of times indoor and outdoor temperatures have been read, for debugging.
uint16_t NtempReads;

// Values of ADC on last indoor and outdoor temperature reads, for debugging.
uint16_t ADClastIndoorTempRead;
uint16_t ADClastOutdoorTempRead;

// Values of computed thermistor resistances on last indoor and outdoor temperature reads,
// for debugging.
uint16_t RlastIndoorTempRead;
uint16_t RlastOutdoorTempRead;

// Values of computed temperatures on last indoor and outdoor temperature reads,
// for debugging.
float TlastIndoorTempRead;
float TlastOutdoorTempRead;

////////////////////////////////////////////////////////////////////////////////////////////
// Initialize for reading indoor and outdoor temperatures. Currently this also initializes
// the ADC converter, which is currently used in this project only for reading thermistors.
/////////////////////////////////////////////////////////////////////////////////////////////
void initReadTemperature(void (*periodicallyCall)()) {
  // First, run a calibration algorithm on the ADC to compute ADC gain and offset constants
  // and load them into the ADC.
  // NOTE: 12-bit ADC resolution is set.
  calibSAMD_ADC_withPWM();

  // Initialize pins.
  pinMode(IndoorThermistor.inputPin, INPUT);
  pinMode(OutdoorThermistor.inputPin, INPUT);

  // Initialize temperature variables, reading initial temperatures and filling the
  // running average buffers with the initial values.
  NtempReads = 0;

  temperature Temp;
  Temp.goingUpC = Temp.goingUpF = true; // Arbitrary at this point.

  readTemperature(IndoorThermistor, Temp, false);
  for (uint8_t i = 0; i < NUM_TEMPS_RUNNING_AVG; i++)
    IndoorTempBuf.Tc[i] = Temp.Tc;
  IndoorTempBuf.idxLatest = 0;
  curIndoorTemperature = Temp;
  ADClastIndoorTempRead = Temp.ADCvalue;
  RlastIndoorTempRead = Temp.Rthermistor;

  readTemperature(OutdoorThermistor, Temp, true);
  for (uint8_t i = 0; i < NUM_TEMPS_RUNNING_AVG; i++)
    OutdoorTempBuf.Tc[i] = Temp.Tc;
  OutdoorTempBuf.idxLatest = 0;
  curOutdoorTemperature = Temp;
  readCurrentTemperatures();
  ADClastOutdoorTempRead = Temp.ADCvalue;
  RlastOutdoorTempRead = Temp.Rthermistor;

  NtempReads = 1;

  // Turn off the AREF output to not warm thermistors.
  digitalWrite(PIN_AREF_OUT, LOW);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Compute rounded version of a floating point temperature value (rounded to an integer).
/////////////////////////////////////////////////////////////////////////////////////////////
int16_t roundTemperature(float Temp, bool goingUp, bool isCelsius) {
  float halfHysteresis = isCelsius ? TEMP_HYST_C/2 : TEMP_HYST_F/2;
  // To round up at 0.375, we must add (1-0.375) and take the floor.  floor(70.375+0.625) = 71.
  return((int16_t) floor(Temp + 0.5 + (goingUp ? halfHysteresis : -halfHysteresis)));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Read temperature from the specified thermistor and return results in Temp.
// On call, Temp contains valid "goingUpC" and "goingUpF" values that are used by
// roundTemperature() to do its rounding.
/////////////////////////////////////////////////////////////////////////////////////////////
void readTemperature(const thermistor& Thermistor, temperature& Temp, bool turnAREFoff) {

  // Skip activating AREF pin if it was left high on previous exit from this function.
  if (digitalRead(PIN_AREF_OUT) == LOW) {
    digitalWrite(PIN_AREF_OUT, HIGH);
    delay(AREF_STABLE_DELAY);
  }

  // Read ADC input.
  #if USE_ANALOG_SAMD
  // Use our revised analogRead() so we can read from D4.
  uint16_t Vo = analogRead_SAMD_TT(Thermistor.inputPin);
  #else
  uint16_t Vo = analogRead(Thermistor.inputPin);
  #endif

  // Turn off AREF if requested.
  if (turnAREFoff)
    digitalWrite(PIN_AREF_OUT, LOW);

  // Compute temperature from voltage.
  uint16_t analogMax = (uint16_t) ADC_MAX;
  float R2 = (float)Thermistor.seriesResistor * ((float)analogMax / (float)Vo - 1.0);
  float logR2 = log(R2);
  float T = (1.0 / (Thermistor.A + Thermistor.B*logR2 + Thermistor.C*logR2*logR2*logR2));
  // Other temperatures are computed from Tc.
  Temp.Tc = T - 273.15;
  // Force an outdoor temperature for debugging.
  #if FORCE_OUTDOOR_TEMP != 9999
  if (&Thermistor == &OutdoorThermistor)
    Temp.Tc = FORCE_OUTDOOR_TEMP;
  #endif
  Temp.Tc_int16 = roundTemperature(Temp.Tc, Temp.goingUpC, true);
  Temp.Tf = degCtoF(Temp.Tc);
  Temp.Tf_int16 = roundTemperature(Temp.Tf, Temp.goingUpF, false);
  Temp.ADCvalue = Vo;
  Temp.Rthermistor = (uint16_t) R2;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Read next temperature and update running average. Also, update goingUpC and goingUpF.
//
// This returns the new temperature that was read (in Celsius) and added to the buffer.
/////////////////////////////////////////////////////////////////////////////////////////////
float readTemperatureRunningAverage(const thermistor& Thermistor,
  temperatureBuf& TempBuf, temperature& Temp, bool turnAREFoff) {

  temperature NewTemp = Temp;

  // Read current temperature and add it to TempBuf.
  readTemperature(Thermistor, NewTemp, turnAREFoff);
  float returnT = NewTemp.Tc;
  uint8_t idxNewTemp = TempBuf.idxLatest + 1;
  if (idxNewTemp >= NUM_TEMPS_RUNNING_AVG)
    idxNewTemp = 0;
  TempBuf.Tc[idxNewTemp] = NewTemp.Tc;
  TempBuf.idxLatest = idxNewTemp;

  // Compute new running average.
  float TcSum = 0.0;
  for (uint8_t i = 0; i < NUM_TEMPS_RUNNING_AVG; i++)
    TcSum += TempBuf.Tc[i];
  NewTemp.Tc = TcSum/NUM_TEMPS_RUNNING_AVG;

  // Compute integer temperatures, rounded.
  NewTemp.Tc_int16 = roundTemperature(NewTemp.Tc, NewTemp.goingUpC, true);
  NewTemp.Tf = degCtoF(NewTemp.Tc);
  NewTemp.Tf_int16 = roundTemperature(NewTemp.Tf, NewTemp.goingUpF, false);
  if (NewTemp.Tc_int16 < Temp.Tc_int16)
    NewTemp.goingUpC = false;
  else if (NewTemp.Tc_int16 > Temp.Tc_int16)
    NewTemp.goingUpC = true;
  if (NewTemp.Tf_int16 < Temp.Tf_int16)
    NewTemp.goingUpF = false;
  else if (NewTemp.Tf_int16 > Temp.Tf_int16)
    NewTemp.goingUpF = true;
  // (NewTemp.ADCvalue was set to last ADC value by readTemperature).
  // (NewTemp.Rthermistor was set to last thermistor resistance by readTemperature).
  Temp = NewTemp;
  return(returnT);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Read indoor and outdoor temperatures, updating the running averages and storing them in
// curIndoorTemperature and curOutdoorTemperature. NtempReads is incremented,
// ADClastIndoorTempRead and ADClastOutdoorTempRead are set to the most recent ADC value,
// RlastIndoorTempRead and RlastOutdoorTempRead are set to the most recent computed
// Rthermistor value, and TlastIndoorTempRead and TlastOutdoorTempRead are set to the most
// recent computed temperature in Fahrenheit.
/////////////////////////////////////////////////////////////////////////////////////////////
void readCurrentTemperatures(void) {
  TlastIndoorTempRead = readTemperatureRunningAverage(IndoorThermistor, IndoorTempBuf,
    curIndoorTemperature, false);
  ADClastIndoorTempRead = curIndoorTemperature.ADCvalue;
  RlastIndoorTempRead = curIndoorTemperature.Rthermistor;

  TlastOutdoorTempRead = readTemperatureRunningAverage(OutdoorThermistor, OutdoorTempBuf,
    curOutdoorTemperature, true);
  ADClastOutdoorTempRead = curOutdoorTemperature.ADCvalue;
  RlastOutdoorTempRead = curOutdoorTemperature.Rthermistor;

  NtempReads++;
  readAndShowCurrentTemperatures();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Display a temperature on the Arduino IDE serial monitor, with a prefix description string.
/////////////////////////////////////////////////////////////////////////////////////////////
void showTemperature(const temperature Temp, char* Desc) {
  // By default printf does not include floating point support, and I can't figure out how to enable it.
  //printf("Temperature: %4.1f°F   %4.1f°C\n", Tf, Tc);
  char TfS[9], TcS[9];
  floatToString(Temp.Tf, TfS, sizeof(TfS), 1);
  floatToString(Temp.Tc, TcS, sizeof(TcS), 1);
  monitor.printf("%s Temperature: %s°F  %s°C   Rthermistor: %ld\n", Desc, TfS, TcS, Temp.Rthermistor);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Read indoor and outdoor temperatures using readCurrentTemperatures() and write them to the
// serial monitor.
/////////////////////////////////////////////////////////////////////////////////////////////
void readAndShowCurrentTemperatures(void) {
  temperature Temp;
  readTemperature(IndoorThermistor, Temp, false);
  showTemperature(Temp, "Indoor");
  readTemperature(OutdoorThermistor, Temp, true);
  showTemperature(Temp, "Outdoor");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
