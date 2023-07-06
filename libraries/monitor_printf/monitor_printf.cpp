/*
  monitor_printf.cpp
*/
#include <stdio.h>
#include <stdarg.h>
#include <Arduino.h>
#include "monitor_printf.h"

static uint16_t printfBufSize = 150;
static char* printfBuf = NULL;

void monitor_init(uint16_t speed, uint16_t printf_buf_size) {
#if MONITOR
  printfBufSize = printf_buf_size;
  printfBuf = (char*) malloc(printfBufSize);
  delay(1000);
  Serial.begin(speed);
  while (!Serial);
  delay(200);
#endif
}

void monitor_printf(const char* format, ...) {
#if MONITOR
  va_list args;
  va_start(args, format);
  vsnprintf(printfBuf, printfBufSize, format, args);
  va_end(args);
  Serial.write(printfBuf);
#endif
}
