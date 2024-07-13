#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include <Arduino.h>
#include <UnixTime.h>
#include <RTClib.h>
#include <stdint.h>
#include "debug_output.h"

struct DateData {
    uint32_t timestamp;
    int8_t zone;
};

// SDA on A4, SCL on A5, SQW on D4
extern RTC_DS3231 rtc;

UnixTime date_time_to_unix_time(int8_t gmt, DateTime date_time);

DateTime unix_time_to_date_time(UnixTime time);

DateTime date_data_to_date_time(DateData date_data);

uint32_t unix_time_to_epoch_time(UnixTime unix_time, uint32_t epoch);

void rtc_setup();

#endif