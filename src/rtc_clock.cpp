#include "rtc_clock.h"

RTC_DS3231 rtc;

/**
 * Converts DateTime to UnixStamp.
*/
UnixStamp date_time_to_unix_time(int8_t gmt, DateTime date_time) 
{
  UnixStamp unix_time(date_time.unixtime());
  return unix_time;
}

/**
 * Converts UnixtTime to DateTime
*/
DateTime unix_time_to_date_time(UnixStamp time)
{
  DateTime dateTime(time.getUnix());
  return dateTime;
}

/**
 * Converts DateData to DateTime
*/
DateTime date_data_to_date_time(DateData date_data)
{
  DateTime dateTime(date_data.timestamp);
  return dateTime;
}


uint32_t unix_time_to_epoch_time(UnixStamp unix_time, uint32_t epoch)
{
  return unix_time.getUnix() - epoch;
}

/**
 * Setup DS3231:
 * - connect
 * - setup time if power lost
 * - clean alarm registers
 * - set 1Hz on SQW pin
 * - assign 1Hz interruption handler
 */
void rtc_setup()
{
  // connect via I2C to the ds3221
  while (!rtc.begin())
  {
    debug_output("Couldn't connect to the ds3221");
    Serial.flush();
    _delay_ms(10);
  }

  // setup compile time if there were powered off
  if (rtc.lostPower())
  {
    debug_output("RTC lost power, setup compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  else
  {
    debug_output("RTC hasnt lost power");
  }

  // we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // clean alarms and disable them, because both registers aren't reset on reboot
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  // start oscilaating at SQW with 1Hz
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
}