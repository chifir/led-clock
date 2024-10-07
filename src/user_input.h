#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <UnixStamp.hpp>
#include <EncButton.h>
#include "debug_output.h"
#include "stdint.h"
#include "matrix_display.h"
#include "rtc_clock.h"

const uint8_t MENU_THRESSHOLD = 5;

UnixStamp user_input_time(civil_time src_time, int8_t src_time_zone, RTC_DS3231 rtc, Button plus_button, Button minus_button);

#endif