/*
  msToString.cpp
*/
#include <stdio.h>
#include "msToString.h"

void msToString(uint32_t MS, char S[MS_TO_STRING_BUF_SIZE]) {
  uint8_t hours = MS / 3600000UL; // That many milliseconds in an hour.
  MS -= hours * 3600000UL;
  if (hours > 99) hours = 99;
  uint8_t minutes = MS / 60000; // That many milliseconds in a minute.
  MS -= minutes * 60000;
  uint8_t seconds = MS / 1000; // That many milliseconds in a second.
  sprintf(S, "%02d:%02d:%02d", hours, minutes, seconds);
}
