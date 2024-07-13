#ifndef MATRIX_DISPLAY_H
#define MATRIX_DISPLAY_H

#include <stdint.h>
#include <WString.h>
#include <GyverMAX7219.h>
#include <RTClib.h>

void display_setup();

void debug_matrix_output(char *msg, double delay);

void debug_matrix_output(String msg, double delay);

void graph_sin(uint8_t offset);

void display_bin(uint32_t time);

void display_time(uint32_t time_to_display, uint8_t mode, RTC_DS3231 rtc);

#endif