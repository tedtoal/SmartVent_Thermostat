/*
  floatToString.cpp
*/
#include <stdlib.h>
#include <stdio.h>
#include "floatToString.h"

void floatToString(float f, char* S, size_t n, int digitsAfterDP) {
  int M = (int) f;
  long D = 1;
  for (int i = digitsAfterDP; i > 0; --i) D *= 10;
  int E = (int) ((abs(f) - abs(M)) * D);
  if (digitsAfterDP == 0)
    snprintf(S, n, "%d", M);
  else {
    char fmt[10]; // "%d.%05d"
    sprintf(fmt, "%%d.%%0%dd", digitsAfterDP);
    snprintf (S, n, fmt, M, E);
  }
}
