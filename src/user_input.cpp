#include "user_input.h"

const char* USER_INPUT_TZ_UTC = "TZ UTC(%d)";
const char* USER_INPUT_YEAR = "YEAR: %d";
const char* USER_INPUT_MONTH = "MONTH: %d";
const char* USER_INPUT_DAY = "DAY: %d";
const char* USER_INPUT_HOUR = "HOUR: %d";
const char* USER_INPUT_MINUTE = "MINUTE: %d";

/**
 * Checks user input, min and max are range boundaries, included.
 * If user input is not in the range, min or max is returned.
 */
int16_t check_user_input(int16_t min, int16_t max, int16_t input)
{
  return (input < min) ? max - input : ((input > max) ? (min + input - max) : input);
}

/**
 * Enter a value.
 */
int16_t user_inupt(const char *msg_template, int16_t min, int16_t max, int16_t current, RTC_DS3231 rtc, Button plus_button, Button minus_button)
{
  debug_output("user input");
  uint32_t menu_seconds = rtc.now().secondstime();
  char *msg;
  do
  {
    plus_button.clear();
    minus_button.clear();
    plus_button.tick();
    minus_button.tick();

    msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, msg_template, current);
    debug_matrix_output(msg, -1);

    if (plus_button.hasClicks() || minus_button.hasClicks())
    {
      current = current + plus_button.getClicks() - minus_button.getClicks();
      menu_seconds = rtc.now().secondstime();
    }

    current = check_user_input(min, max, current);

    free(msg);
  } while ((rtc.now().secondstime() - menu_seconds) < MENU_THRESSHOLD);
  debug_output(current);
  free(msg);
  return current;
}

/**
 * Check leap year
 */
bool is_leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * Get max days in monty
 */
uint8_t get_days_in_month(uint8_t month, uint16_t year)
{
  const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month == 2 && is_leap_year(year))
  {
    return days_in_month[month - 1] + 1;
  }

  return days_in_month[month - 1];
}

/**
 * Get and converts user input into unixtime object.
 */
DateData user_input_unix_time(UnixTime src_time, int8_t src_time_zone, RTC_DS3231 rtc, Button plus_button, Button minus_button)
{
  debug_output("user_input_unix_time");
  DateData src_date_data;
  src_date_data.timestamp = src_time.getUnix();
  src_date_data.zone = src_time_zone;
  do
  {
    int8_t time_zone = (int8_t)user_inupt(USER_INPUT_TZ_UTC, -11, 12, (int16_t)src_time_zone, rtc, plus_button, minus_button);
    uint16_t year = (uint16_t)user_inupt(USER_INPUT_YEAR, 1970, 2099, (int16_t)src_time.year, rtc, plus_button, minus_button);
    uint8_t month = (uint8_t)user_inupt(USER_INPUT_MONTH, 1, 12, (int16_t)src_time.month, rtc, plus_button, minus_button);
    uint8_t max_day = (uint8_t)get_days_in_month(month, year);
    uint8_t day = (uint8_t)user_inupt(USER_INPUT_DAY, 1, max_day, (int16_t)src_time.day, rtc, plus_button, minus_button);
    uint8_t hour = (uint8_t)user_inupt(USER_INPUT_HOUR, 0, 23, (int16_t)src_time.hour, rtc, plus_button, minus_button);
    uint8_t minute = (uint8_t)user_inupt(USER_INPUT_MINUTE, 0, 59, (int16_t)src_time.minute, rtc, plus_button, minus_button);
    DateData user_date;
    user_date.zone = time_zone;
    
    UnixTime unix_time(time_zone);
    unix_time.setDateTime(year, month, day, hour, minute, 0);
    
    user_date.timestamp = unix_time.getUnix();
    return user_date;
  } while (true);

  return src_date_data;
}