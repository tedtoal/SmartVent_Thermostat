/*
  msToString.h

  A function to convert a number of milliseconds into a time string of the
  form HH:MM:SS in S.
*/
#include <Arduino.h>

#define MS_TO_STRING_BUF_SIZE 9

extern void msToString(uint32_t MS, char S[MS_TO_STRING_BUF_SIZE]);
