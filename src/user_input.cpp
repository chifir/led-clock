#include "user_input.h"

enum input_field
{
  tz = 0,
  year = 1,
  mon = 2,
  day = 3,
  hour = 4,
  min = 5
};

/**
 * Checks user input, min and max are range boundaries, included.
 * If user input is not in the range, min or max is returned.
 */
int16_t check_user_input(int16_t min, int16_t max, int16_t input)
{
  return (input < min) ? max - input : ((input > max) ? (min + input - max) : input);
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

char* get_msg_template(civil_time time, input_field field) 
{
  const char *templates[] = {
    "TZ UTC(%02d)", 
    "%s/%02u/%02u %02u:%02u", 
    "%04u/%s/%02u %02u:%02u", 
    "%04u/%02u/%s %02u:%02u", 
    "%04u/%02u/%02u %s:%02u", 
    "%04u/%02u/%02u %02u:%s"
    };
  char *msg_template = (char *)calloc(32, sizeof(char));
  switch (field)
  {
  case input_field::tz: 
  {
    return const_cast<char*>(templates[field]);
  }
    break;
  case input_field::year: 
  {
    sprintf(msg_template, templates[field], "%04u", time.mon, time.day, time.hour, time.min);
  }
    break;
  case input_field::mon:
  {
    sprintf(msg_template, templates[field], time.year, "%02u", time.day, time.hour, time.min);
  }
    break;
  case input_field::day:
  {
    sprintf(msg_template, templates[field], time.year, time.mon, "%02u", time.hour, time.min);
  }
    break;
  case input_field::hour:
  {
    sprintf(msg_template, templates[field], time.year, time.mon, time.day, "%02u", time.min);
  }
    break;
  case input_field::min:
  {
    sprintf(msg_template, templates[field], time.year, time.mon, time.day, time.hour, "%02u");
  }
    break;
  default:
    msg_template = NULL;
    break;
  }
  return msg_template;
}

/**
 * Enter a value.
 */
int16_t user_input(civil_time time, input_field field, int16_t min, int16_t max, int16_t current, RTC_DS3231 *rtc, Button *position_button, Button *plus_button, Button *minus_button)
{
  uint32_t menu_seconds = rtc->now().secondstime();
  char *msg, *msg_template;
  msg_template = get_msg_template(time, field);
  msg = (char *)calloc(18, sizeof(char));
  sprintf(msg, msg_template, 0);
  do
  {
    plus_button->clear();
    minus_button->clear();
    plus_button->tick();
    minus_button->tick();
    position_button->clear();
    position_button->tick();

    matrix_display_string(msg);

    if (plus_button->hasClicks() || minus_button->hasClicks())
    {
      current += plus_button->getClicks() - minus_button->getClicks();
      current = check_user_input(min, max, current);
      menu_seconds = rtc->now().secondstime();
      free(msg);
      msg = (char *)calloc(18, sizeof(char));
      sprintf(msg, msg_template, current);
    }

    if (position_button->hasClicks())
    {
      return current;
    }

  } while ((rtc->now().secondstime() - menu_seconds) < MENU_THRESSHOLD);

  free(msg);
  free(msg_template);
  return current;
}

/**
 * Get and converts user input into unixtime object.
 */
UnixStamp user_input_time(civil_time time, int8_t time_zone, RTC_DS3231 *rtc, Button *next_position_button, Button *plus_button, Button *minus_button)
{
  time_zone = (int8_t)user_input(time, input_field::tz, -11, 12, (int16_t)time_zone, rtc, next_position_button, plus_button, minus_button);
  time.year = (uint16_t)user_input(time, input_field::year, 1970, 2099, (int16_t)time.year, rtc, next_position_button, plus_button, minus_button);
  time.mon = (uint8_t)user_input(time, input_field::mon, 1, 12, (int16_t)time.mon, rtc, next_position_button, plus_button, minus_button);
  uint8_t max_day = get_days_in_month(time.mon, time.year);
  time.day = (uint8_t)user_input(time, input_field::day, 1, max_day, (int16_t)time.day, rtc, next_position_button, plus_button, minus_button);
  time.hour = (uint8_t)user_input(time, input_field::hour, 0, 23, (int16_t)time.hour, rtc, next_position_button, plus_button, minus_button);
  time.min = (uint8_t)user_input(time, input_field::min, 0, 59, (int16_t)time.min, rtc, next_position_button, plus_button, minus_button);

  UnixStamp unix_stamp(time, time_zone);
  return unix_stamp;
}