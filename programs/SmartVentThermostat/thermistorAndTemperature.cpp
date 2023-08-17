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
#include <calibSAMD_ADC_withPWM.h>

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

// pinAREF_OUT
static pin_size_t PIN_AREF_OUT;

////////////////////////////////////////////////////////////////////////////////////////////
// Initialize for reading indoor and outdoor temperatures. Currently this also initializes
// the ADC converter, which is currently used in this project only for reading thermistors.
/////////////////////////////////////////////////////////////////////////////////////////////
void initReadTemperature(int pinADC_CALIB, int pinPWM_CALIB, int pinAREF_OUT,
  uint8_t cfgADCmultSampAvg, void (*periodicallyCall)()) {
  // Save AREF_OUT pin number.
  PIN_AREF_OUT = pinAREF_OUT;

  // Run a calibration algorithm on the ADC to compute ADC gain and offset constants
  // and load them into the ADC.
  // NOTE: 12-bit ADC resolution is set.
  calibSAMD_ADC_withPWM(pinADC_CALIB, pinPWM_CALIB, pinAREF_OUT, cfgADCmultSampAvg);

  // Initialize pins.
  pinMode(IndoorThermistor.inputPin, INPUT);
  pinMode(OutdoorThermistor.inputPin, INPUT);

  // Count total number of temperature reads, for debugging.
  NtempReads = 0;

  // Initialize goingUpC/goingUpF true, which is arbitrary at this point.
  temperature Temp;
  Temp.goingUpC = Temp.goingUpF = true;

  // Read indoor temperature.
  readTemperature(IndoorThermistor, Temp, false);

  // Force an indoor initialization temperature for debugging.
  #if FORCE_INDOOR_TEMP != 9999
  Temp.Tc = Temp.Tc_int16 = (FORCE_INDOOR_TEMP + DEBUG_TEMP_OFFSET);
  Temp.Tf = Temp.Tf_int16 = degCtoF(Temp.Tc_int16);
  #endif

  // Fill the indoor running average buffer with the initial value.
  for (uint8_t i = 0; i < NUM_TEMPS_RUNNING_AVG; i++)
    IndoorTempBuf.Tc[i] = Temp.Tc;
  IndoorTempBuf.idxLatest = 0;
  curIndoorTemperature = Temp;
  ADClastIndoorTempRead = Temp.ADCvalue;
  RlastIndoorTempRead = Temp.Rthermistor;

  // Read outdoor temperature.
  readTemperature(OutdoorThermistor, Temp, true);

  // Force an outdoor initialization temperature for debugging.
  #if FORCE_OUTDOOR_TEMP != 9999
  Temp.Tc = Temp.Tc_int16 = (FORCE_OUTDOOR_TEMP + DEBUG_TEMP_OFFSET);
  Temp.Tf = Temp.Tf_int16 = degCtoF(Temp.Tc_int16);
  #endif

  // Fill the outdoor running average buffer with the initial value.
  for (uint8_t i = 0; i < NUM_TEMPS_RUNNING_AVG; i++)
    OutdoorTempBuf.Tc[i] = Temp.Tc;
  OutdoorTempBuf.idxLatest = 0;
  curOutdoorTemperature = Temp;
  readCurrentTemperatures();
  ADClastOutdoorTempRead = Temp.ADCvalue;
  RlastOutdoorTempRead = Temp.Rthermistor;

  // We've now read temperatures one time.
  NtempReads = 1;

  // Turn off the AREF output to not warm thermistors.
  digitalWrite(PIN_AREF_OUT, LOW);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Compute rounded version of a floating point temperature value (rounded to an integer).
/////////////////////////////////////////////////////////////////////////////////////////////
int16_t roundTemperature(float Temp, bool goingUp, bool isCelsius) {
  float halfHysteresis = isCelsius ? TEMP_HYST_C/2 : TEMP_HYST_F/2;
  // From the comments for this function in the .h file:
  //    Say the rounded temperature is 70 and goingUp is true. Then it will increase to 71 at
  //    70.375 and it will decrease to 69 at 69.374. Say instead the reading is 69.374, which
  //    is rounded to 69, and goingUp becomes false. The new threshold is: 0.5 + 0.125 = 0.625.
  //    The rounded temperature will now decrease one degree further to 68 at 68.624, but it
  //    will not increase BACK to 70 until the temperature reaches 69.625. 
  //
  // How do we round at a particular threshold such as .375? If we use the round() function,
  // this creates issues when the temperature crosses 0°. To avoid that, we will instead use
  // the floor function, which always rounds in the more negative direction, and it doesn't
  // use a threshold of 0.5 like round(), but instead the threshold is 0, i.e. any fractional
  // part is discarded, so floor(70.1) = 70 and floor(-10.1) = -11.
  //
  // So, if we are using the floor() function and it has that threshold of 0, how do we make it
  // instead have a threshold of f, where f is e.g. (0.5 + goingUp ? -0.125 : +0.125) from the
  // above example? The answer is that we must ADD (1-threshold) to the number to be rounded.
  //
  // So, if the number is 70.626 and threshold is 0.625 (goingUp is false, i.e. it is going
  // down), compute 70.626 + 1 - 0.625 = 71.001 and take the floor of that, giving 71. At
  // 70.624, compute 70.624 + 1 - 0.625 = 70.999 and take the floor to get 70. As expected,
  // when integer temperature is 70 and going down, actual temperature has to go up to a
  // threshold of 70.625, greater than 70.5, for integer temperature to go up to 71. (When
  // goingUp was true and temperature was 71, it had to drop to 70.375 for integer temperature
  // to drop to 70 and goingUp to become false. It might jitter but hopefully not so far as
  // back up to 70.625).
  //
  // Summary: add 1-threshold to number before applying floor.
  // Threshold: 0.5 + (goingUp ? -hysteresis/2 : +hysteresis/2)
  // Operation: floor(temp + 1 - (0.5 + (goingUp ? -hysteresis/2 : +hysteresis/2)))
  //          = floor(temp + 0.5 - (goingUp ? -hysteresis/2 : +hysteresis/2))
  return((int16_t) floor(Temp + 0.5 - (goingUp ? -halfHysteresis : +halfHysteresis)));
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
  if (Vo <= 5) Vo = 5; // Avoid ridiculously small Vo.
  float R2 = (float)Thermistor.seriesResistor * ((float)analogMax / (float)Vo - 1.0);
  float logR2 = log(R2);
  float Tk = (1.0 / (Thermistor.A + Thermistor.B*logR2 + Thermistor.C*logR2*logR2*logR2));
  // Other temperatures are computed from Tc.
  Temp.Tc = degKtoC(Tk);

  // Force an indoor temperature for debugging.
  #if FORCE_INDOOR_TEMP != 9999
  if (&Thermistor == &IndoorThermistor)
    Temp.Tc = FORCE_INDOOR_TEMP;
  #endif
  
  // Force an outdoor temperature for debugging.
  #if FORCE_OUTDOOR_TEMP != 9999
  if (&Thermistor == &OutdoorThermistor)
    Temp.Tc = FORCE_OUTDOOR_TEMP;
  #endif

  // Compute other temperatures from Temp.Tc.
  Temp.Tc_int16 = roundTemperature(Temp.Tc, Temp.goingUpC, true);
  Temp.Tf = degCtoF(Temp.Tc);
  Temp.Tf_int16 = roundTemperature(Temp.Tf, Temp.goingUpF, false);
  Temp.ADCvalue = Vo;
  Temp.Rthermistor = (uint16_t) R2;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Read next temperature and update running average in Temp. Also, update goingUpC and
// goingUpF.
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
  // Set argument Temp (a reference) to the new average temperature.
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
