/*
  floatToString.h

  A function to convert a floating point value f into a string S (of size n)
  with digitsAfterDP digits after the decimal point.
*/
#include <Arduino.h>

extern void floatToString(float f, char* S, size_t n, int digitsAfterDP);
