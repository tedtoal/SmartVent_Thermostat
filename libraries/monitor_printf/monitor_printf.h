/*
  monitor_printf.h

  Function monitor_printf() is defined, providing a way to do printf's to the
  serial monitor. Call monitor_initialize first thing in setup().
*/
#include <Arduino.h>

// Set this 1 to enable monitor_printf and serial monitor output, 0 to disable.
// Be sure to set to 0 for running without USB connection!
// monitor_printf.h defines an empty monitor_printf() function if MONITOR is 0.
#define MONITOR 0

extern void monitor_init(uint16_t speed=115200, uint16_t printf_buf_size=150);
extern void monitor_printf(const char* format, ...);
