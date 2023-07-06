//******************************************************************************
// calibADC_gain_offset_withPWM
// Author: Ted Toal
// Date: 21-May-2023
//
// Implementation file for functions declared in calibADC_gain_offset_withPWM.h.
//******************************************************************************

#include <Arduino.h>
#include <wiring_private.h>
#include <analog_D4567.h>       // Modified version of wiring_analog.h, supports changing period of PWM waveform, supports using D4-D7 pins.
#if USE_MONITOR_PRINTF
#include <monitor_printf.h>     // See #define MONITOR in monitor_printf.h for disabling use of the serial port when no USB port is being used (live code).
#endif
#include <calibADC_gain_offset_withPWM.h>

// Gain correction value of 1 = no correction.
#define GAIN_CORR_1 0x800

//******************************************************************************
// Read the ADC value at PERCENT_AT_ENDS and 100-PERCENT_AT_ENDS of ref voltage.
//******************************************************************************
void readADCatEnds(int* ADClow, int* ADChigh) {
  analogUpdatePWM_TCC_D4567(PIN_PWM, 10*PERCENT_AT_ENDS, 1000);
  delay(PWM_STABLE_DELAY);
  *ADClow = analogRead_D4567(PIN_CAP);
  analogUpdatePWM_TCC_D4567(PIN_PWM, 1000-10*PERCENT_AT_ENDS, 1000);
  delay(PWM_STABLE_DELAY);
  *ADChigh = analogRead_D4567(PIN_CAP);

  #if USE_MONITOR_PRINTF
  // Show measured values.
  monitor_printf(" At %2d%% of VREF, expected ADC = %5d, actual ADC = %5d\n",
    PERCENT_AT_ENDS, (PERCENT_AT_ENDS*ADC_MAX + 50)/100, *ADClow);
  monitor_printf(" At %2d%% of VREF, expected ADC = %5d, actual ADC = %5d\n",
    100-PERCENT_AT_ENDS, ((100-PERCENT_AT_ENDS)*ADC_MAX + 50)/100, *ADChigh);
  #endif
}

//******************************************************************************
// Calibration function that computes and loads gain and offset error corrections.
//******************************************************************************
extern void fixADC_GainOffset() {
  int ADClow, ADChigh;

  #if USE_MONITOR_PRINTF
  // Show the current values of the ADC CALIB, GAINCORR, and OFFSETCORR registers.
  monitor_printf("ADC CALIB = %04X\n", ADC->CALIB.reg);
  monitor_printf("ADC GAINCORR = %04X\n", ADC->GAINCORR.reg);
  monitor_printf("ADC OFFSETCORR = %04X\n", ADC->OFFSETCORR.reg);
  #endif

  #if PIN_AREF_OUT != -1
  // Initialize the digital output that connects to the AREF input and drive it high.
  pinMode(PIN_AREF_OUT, OUTPUT);
  digitalWrite(PIN_AREF_OUT, HIGH);
  delay(AREF_STABLE_DELAY);
  #endif

  // Connect PIN_CAP to the ADC.
  pinPeripheral(PIN_CAP, PIO_ANALOG);

  // Select 16-bit PWM resolution (only need 8, but easiest to stick with 16).
  analogWriteResolution_PWM_D4567(16);

  // Initialize PWM TCC. We use a period of 1000 to make percentage easy.
  analogStartPWM_TCC_D4567(PIN_PWM, 0, 1000);

  // Select AREF-A as external voltage reference for the ADC.
  analogReference_D4567(AR_EXTERNAL);

  // Select 12-bit ADC resolution.
  analogReadResolution_D4567(12);

  // Load an initial gain and offset error of NO CORRECTION.
  ADC->GAINCORR.reg = ADC_GAINCORR_GAINCORR(GAIN_CORR_1);
  ADC->OFFSETCORR.reg = ADC_OFFSETCORR_OFFSETCORR(0);
  ADC->CTRLB.bit.CORREN = true;

  #if CFG_ADC_MULT_SAMP_AVG
  // Configure multiple sampling and averaging.
  // We set ADJRES according to table 33-3 in the datasheet.
  uint8_t adjRes = (CFG_ADC_MULT_SAMP_AVG <= 4) ? CFG_ADC_MULT_SAMP_AVG : 4;
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM(CFG_ADC_MULT_SAMP_AVG) | ADC_AVGCTRL_ADJRES(adjRes);
  ADC->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_16BIT;
  #endif

  // Determine gain error by measuring PERCENT_AT_ENDS and 100-PERCENT_AT_ENDS of
  // reference voltage.
  #if USE_MONITOR_PRINTF
  monitor_printf("Read ADC to compute gain error\n");
  #endif
  readADCatEnds(&ADClow, &ADChigh);

  // The actual gain is the slope: ((ADChigh-ADClow)*100)/(ADC_MAX*(100-2*PERCENT_AT_ENDS))
  // The gain error is the inverse: (ADC_MAX*(100-2*PERCENT_AT_ENDS))/((ADChigh-ADClow)*100)
  // Multiply by GAIN_CORR_1 to scale it to a 1.11 bit number:
  //    (GAIN_CORR_1*ADC_MAX*(100-2*PERCENT_AT_ENDS))/((ADChigh-ADClow)*100)
  // I was going to perform rounding by adding half of the divisor before the divide,
  // but it doesn't really seem to improve the result.
  uint32_t divisor1 = (uint32_t)(ADChigh-ADClow)*100;
  uint16_t gainError = (uint16_t) ((uint32_t)(GAIN_CORR_1*ADC_MAX*(100-2*PERCENT_AT_ENDS))/divisor1);
  #if USE_MONITOR_PRINTF
  monitor_printf("gainError = %d\n", gainError);
  #endif

  // Load the gain error into the ADC.
  ADC->GAINCORR.reg = ADC_GAINCORR_GAINCORR(gainError);
  ADC->OFFSETCORR.reg = ADC_OFFSETCORR_OFFSETCORR(0);
  ADC->CTRLB.bit.CORREN = true;

  // Determine offset error by again measuring PERCENT_AT_ENDS and 100-PERCENT_AT_ENDS
  // of reference voltage.
  #if USE_MONITOR_PRINTF
  monitor_printf("Read ADC to compute offset error\n");
  #endif
  readADCatEnds(&ADClow, &ADChigh);

  // The offset error is the negative of the offset at 0 (y-axis intercept):
  //  - ((PERCENT_AT_ENDS*ADChigh - (100-PERCENT_AT_ENDS)*ADClow)/(100-2*PERCENT_AT_ENDS))
  // I was going to perform rounding by adding half of the divisor before the divide,
  // but it doesn't really seem to improve the result.
  int32_t divisor2 = (100-2*PERCENT_AT_ENDS);
  int offsetError = - (int)((((int32_t)PERCENT_AT_ENDS*ADChigh - (int32_t)(100-PERCENT_AT_ENDS)*ADClow))/divisor2);
  #if USE_MONITOR_PRINTF
  monitor_printf("offsetError = %d\n", offsetError);
  #endif

  // Load both the gain and offset error into the ADC.
  ADC->GAINCORR.reg = ADC_GAINCORR_GAINCORR(gainError);
  ADC->OFFSETCORR.reg = ADC_OFFSETCORR_OFFSETCORR(offsetError);
  ADC->CTRLB.bit.CORREN = true;

  #if USE_MONITOR_PRINTF
  // Now see how well we did by again measuring PERCENT_AT_ENDS and 100-PERCENT_AT_ENDS of
  // reference voltage.
  monitor_printf("Read ADC a third time to view results of correction\n");
  readADCatEnds(&ADClow, &ADChigh);
  #endif
}
