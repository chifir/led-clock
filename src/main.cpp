#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>
#include <GyverMAX7219.h>
#include <EncButton.h>
#include <avr/eeprom.h>
#include <UnixTime.h>

#define DEBUG true
#define CLOCK_INTERRUPT_PIN 2
#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_SET_PIN 7
#define EEPROM_GMT_OFFSET 0
#define EPOCH_BEGIN_OFFSET 0
#define CLOCK_OFFSET 1
#define EPOCH_BEGIN 536229000
#define DEFAULT_TIMEZONE 3

// button settings
#define EB_DEB_TIME 50
#define EB_CLICK_TIME 500
#define EB_HOLD_TIME 600
#define EB_STEP_TIME 200

// menu
#define SET_CURRENT_TIME 1
#define SET_EPOCH_TIME 2

const uint8_t DISPLAY_WIDTH = 63;
const uint8_t HEIGHT = 3;
const uint8_t WiDITH = 3;
const uint8_t MODE[5] = {2, 8, 10, 16, 0};
const uint8_t MODE_SIZE = 5;
const uint8_t DEBOUNCE_DELAY_MS = 15;
const uint8_t MENU_THRESSHOLD = 5;

const char* USER_INPUT_TZ_UTC = "TZ UTC(%d)";
const char* USER_INPUT_YEAR = "YEAR: %d";
const char* USER_INPUT_MONTH = "MONTH: %d";
const char* USER_INPUT_DAY = "DAY: %d";
const char* USER_INPUT_HOUR = "HOUR: %d";
const char* USER_INPUT_MINUTE = "MINUTE: %d";

enum display_mode
{
  bin = 2,
  oct = 8,
  dec = 10,
  hex = 16,
  str = 0
};

volatile uint32_t seconds = 0;
volatile uint32_t recovery_epoch_begin_timestamp = 0;
volatile uint32_t recovery_clock_timestamp = 0;
volatile int8_t current_timezone = 0;
volatile bool trigger_display_update = true;
uint32_t menu_seconds = 0;
bool IS_BTNS_PRESSED = false;

uint8_t CURRENT_MODE_INDEX = 0;
uint32_t offset = 0;
u8 deltas_count = 0;

// SDA on A4, SCL on A5, SQW on D4
RTC_DS3231 rtc;
RTC_I2C rtc_i2c;

// 8 matrix in 1 row on D5
MAX7219<12, 1, 5> mtrx;

Button change_mode(BUTTON_CHANGE_MODE_PIN, INPUT_PULLUP, LOW);
Button set_btn(BUTTON_SET_PIN, INPUT_PULLUP, LOW);

void debug_output(const char *msg)
{
  if (DEBUG)
    Serial.println(msg);
}

void debug_output(uint32_t num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_output(uint16_t num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(20, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_output(int num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_matrix_output(char *msg, double delay)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  mtrx.print(msg);
  mtrx.update();
  if (delay != -1)
    _delay_ms(delay);
}

/**
 * Reads GMT from eeprom
*/
int8_t get_timezone()
{
  return (int8_t) eeprom_read_byte(EEPROM_GMT_OFFSET);
}

/**
 * Writes GMT to eeprom
*/
void update_gmt(int8_t gmt) 
{
  eeprom_update_byte(EEPROM_GMT_OFFSET, gmt);
}

/**
 * Reads timestamp(4 bytes) from eeprom by index:
 * 0 - base timestamp
 * 1 - clock timestamp (just for recovering)
*/
uint32_t get_eeprom_timestamp(byte index)
{
  return eeprom_read_dword((const uint32_t *)(1 + 4 * index));
}

/**
 * Wtites timestamp(4 bytes) to eeprom by index:
 * 0 - base timestamp
 * 1 - clock timestamp (just for recovering)
*/
void update_eeprom_timestamp(byte index, uint32_t baseTimestamp)
{
  eeprom_update_dword((uint32_t *)(1 + 4 * index), baseTimestamp);
}

/**
 * Just for fun, writes a sinux graph on the display
*/
void graph_sin(uint8_t offset)
{
  uint8_t x = 0;
  uint8_t y = 0;
  mtrx.clear();
  mtrx.setCursor(0, 0);
  for (uint8_t i = 0 + offset; i <= 95 + offset; i++)
  {
    y = 4 * (1 + sinf(25.7 * i));
    mtrx.dot(x, y);
    x++;
  }
  mtrx.update();
}

/**
 * Converts DateTime to UnixTime.
*/
UnixTime date_time_to_unix_time(int8_t gmt, DateTime date_time) 
{
  UnixTime unix_time(gmt);
  unix_time.getDateTime(date_time.unixtime());
  return unix_time;
}

/**
 * Converts UnixtTime to DateTime
*/
DateTime unix_time_to_date_time(UnixTime time)
{
  DateTime dateTime(time.getUnix());
  return dateTime;
}

uint32_t unix_time_to_epoch_time(UnixTime unix_time)
{
  return unix_time.getUnix() - recovery_epoch_begin_timestamp;
}

/**
 * Displays binary date output.
 */
void display_bin(uint32_t time)
{
  const uint8_t START_POSITION = 16;
  bool bin_time[32];
  uint8_t x = START_POSITION;
  uint8_t y = 0;

  // convert to binary array
  for (uint8_t i = 0; i < sizeof(uint32_t) * 8; i++)
  {
    uint8_t num = (time >> i) & 1;
    bin_time[sizeof(uint32_t) * 8 - 1 - i] = num == 1;
  }

  // print line for 1, rectangle for 0
  for (uint8_t i = 0; i < sizeof(uint32_t) * 8; i++)
  {
    bool tmp_bool = bin_time[i];
    if (tmp_bool)
    {
      mtrx.lineV(x, y, y + HEIGHT - 1);
    }
    else
    {
      mtrx.rectWH(x, y, WiDITH, HEIGHT, GFX_STROKE);
    }
    x = x + WiDITH + 1;
    if (x >= DISPLAY_WIDTH + START_POSITION)
    {
      x = START_POSITION;
      y = HEIGHT + 1;
    }
  }
}

/**
 * Calls appropriate display function by number system.
 */
void display_time(uint32_t time_to_display, uint8_t mode)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  switch (mode)
  {
  case display_mode::bin:
  {
    display_bin(time_to_display);
  }
  break;
  case display_mode::oct:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(time_to_display, display_mode::oct);
  }
  break;
  case display_mode::dec:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(time_to_display, display_mode::dec);
  }
  break;
  case display_mode::hex:
  {
    mtrx.setCursor(27, 0);
    mtrx.print(time_to_display, display_mode::hex);
  }
  break;
  // only for dev and debug
  case display_mode::str:
  {
    char date[17] = "YYYY:MM:DD:hh:mm";
    rtc.now().toString(date);
    mtrx.print(date);
  }
  break;
  default:
    break;
  }
  mtrx.update();
}

/**
 * Update display info.
 */
void display_update()
{
  if (trigger_display_update)
  {
    trigger_display_update = false;
    DateTime now = rtc.now();
    UnixTime unix_time = date_time_to_unix_time(current_timezone, now);
    display_time(unix_time_to_epoch_time(unix_time), MODE[CURRENT_MODE_INDEX]);
  }
}

/**
 * Changes cyclically date format.
 */
void display_format_mode_change()
{
  if (CURRENT_MODE_INDEX >= 0 && CURRENT_MODE_INDEX < MODE_SIZE - 1)
  {
    CURRENT_MODE_INDEX++;
  }
  else if (CURRENT_MODE_INDEX == MODE_SIZE - 1)
  {
    CURRENT_MODE_INDEX = 0;
  }
}

/**
 * Changes date format by button click.
 */
void button_mode_action()
{
  if (change_mode.hasClicks())
  {
    display_format_mode_change();
  }
}

/**
 * SQW signal interruption handler
 *
 * - reads current date from RTC
 * - sends data to the display handler
 */
void rtc_interruption_handler()
{
  trigger_display_update = true;
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

  // assign interruption handler
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), rtc_interruption_handler, FALLING);

  // clean alarms and disable them, because both registers aren't reset on reboot
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  // start oscilaating at SQW with 1Hz
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
}

void display_setup()
{
  mtrx.begin();
  mtrx.clear();
  mtrx.update();
}

/**
 * Writes default base timestamp in EEPROM, if it wasn't set previously.
 * Reads base timestamp from EEPROM.
 */
void setup_from_eeprom()
{
  update_eeprom_timestamp(0, EPOCH_BEGIN);
  
  // read timezone
  current_timezone = get_timezone();
  if (~current_timezone == 0)
  {
    debug_output("timezone wasn't set");
    update_gmt(DEFAULT_TIMEZONE);
    current_timezone = DEFAULT_TIMEZONE;
  }
  // read timestamp for clock
  recovery_clock_timestamp = get_eeprom_timestamp(CLOCK_OFFSET);
  // read and set epoch begining timestamp
  recovery_epoch_begin_timestamp = get_eeprom_timestamp(EPOCH_BEGIN_OFFSET);
  if (~recovery_epoch_begin_timestamp == 0)
  {
    debug_output("begining wasn't set");
    update_eeprom_timestamp(0, EPOCH_BEGIN);
    recovery_epoch_begin_timestamp = EPOCH_BEGIN;
  }
}

void dislpay_timestamps() 
{
  debug_output("EPOCH:");
  Serial.println(get_eeprom_timestamp(EPOCH_BEGIN_OFFSET));
  debug_output("CLOCK");
  Serial.println(get_eeprom_timestamp(CLOCK_OFFSET));
}

void serial_setup() 
{
  if (change_mode.holding())
  {
    //debug_output("serial_setup_start");
    while(Serial.available()) 
    {
      Serial.println("Serial setup");
      String input = Serial.readString();
      Serial.println(input);
      if (input.equals("b")) 
      {
        break;
      }
    }
    //debug_output("serial_setup_end");
  }
  else 
  {
    return;
  }
}

void setup()
{
  if (DEBUG)
  {
    Serial.begin(9800);
  }

  debug_output("EEPROM setup");
  setup_from_eeprom();
  debug_output("start setup");
  debug_output("display setup");
  display_setup();
  debug_output("rtc setup");
  rtc_setup();
  dislpay_timestamps();
  CURRENT_MODE_INDEX = 4;

  serial_setup();
}

//TODO: delete
void print_datetime()
{
  char date[14] = "YYYY:hh:mm:ss";
  rtc.now().toString(date);
  Serial.println(date);
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
 * Checks user input, min and max are range boundaries, included.
 * If user input is not in the range, min or max is returned.
 */
int16_t check_user_input(int16_t min, int16_t max, int16_t input)
{
  return (input < min) ? min : ((input > max) ? max : input);
}

/**
 * Enter a value.
 */
int16_t user_inupt(const char *msg_template, int16_t min, int16_t max, int16_t current)
{
  debug_output("user input");
  menu_seconds = rtc.now().secondstime();
  char *msg;
  do
  {
    set_btn.clear();
    change_mode.clear();
    set_btn.tick();
    change_mode.tick();

    msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, msg_template, current);
    debug_matrix_output(msg, -1);

    if (set_btn.hasClicks() || change_mode.hasClicks())
    {
      current = current + set_btn.getClicks() - change_mode.getClicks();
    }

    if (set_btn.hasClicks() || change_mode.hasClicks())
    {
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
 * Get and converts user input into unixtime object.
 */
UnixTime user_input_unix_time(UnixTime src_time, int8_t src_time_zone)
{
  debug_output("user_input_unix_time");
  do
  {
    int8_t time_zone = (int8_t)user_inupt(USER_INPUT_TZ_UTC, -11, 12, (int16_t)src_time_zone);
    uint16_t year = (uint16_t)user_inupt(USER_INPUT_YEAR, 1970, 2099, (int16_t)src_time.year);
    uint8_t month = (uint8_t)user_inupt(USER_INPUT_MONTH, 1, 12, (int16_t)src_time.month);
    uint8_t max_day = (uint8_t)get_days_in_month(month, year);
    uint8_t day = (uint8_t)user_inupt(USER_INPUT_DAY, 1, max_day, (int16_t)src_time.day);
    uint8_t hour = (uint8_t)user_inupt(USER_INPUT_HOUR, 0, 23, (int16_t)src_time.hour);
    uint8_t minute = (uint8_t)user_inupt(USER_INPUT_MINUTE, 0, 59, (int16_t)src_time.minute);

    current_timezone = time_zone;
    UnixTime unix_time(time_zone);
    unix_time.setDateTime(year, month, day, hour, minute, 0);
    return unix_time;
  } while (true);

  return src_time;
}


void set_current_time() 
{
  debug_output("set_current_time start");
  // convert to unix
  debug_output("get rtc.now() for converting into ");
  DateTime now = rtc.now();
  debug_output("convert to unix");
  UnixTime current_time = date_time_to_unix_time(current_timezone, now);
  // get time from user
  current_time = user_input_unix_time(current_time, current_timezone);
  // udpate eeprom for recovery
  update_eeprom_timestamp(CLOCK_OFFSET, current_time.getUnix());
  // update rtc clock 
  rtc.adjust(unix_time_to_date_time(current_time));

  dislpay_timestamps();
}

void debug_output_unixtimestamp(UnixTime unix_time)
{
  // debug_output(unix_time.year);
  debug_output(unix_time.month);
  debug_output(unix_time.day);
  debug_output(unix_time.hour);
  debug_output(unix_time.minute);
  // char *msg = (char *)calloc(32, sizeof(char));
  // sprintf(msg, "%d:%d:%d:%d", unix_time.year, unix_time.month, unix_time.hour, unix_time.minute);
  // Serial.println(msg);
  // free(msg);
}

void set_epoch_time()
{
  dislpay_timestamps();
  UnixTime epoch_start(current_timezone);
  epoch_start.getDateTime(get_eeprom_timestamp(EPOCH_BEGIN_OFFSET));
  debug_output("before");
  debug_output_unixtimestamp(epoch_start);
  // get time from user
  UnixTime user_epoch_start = user_input_unix_time(epoch_start, current_timezone);
  debug_output("after");
  Serial.println(user_epoch_start.getUnix());
  debug_output_unixtimestamp(user_epoch_start);
  // udpate eeprom for recovery
  update_eeprom_timestamp(EPOCH_BEGIN_OFFSET, user_epoch_start.getUnix());
  dislpay_timestamps();
}

void menu_action(uint8_t option)
{
  switch (option)
  {
  case SET_CURRENT_TIME:
  {
    set_current_time();
  }
    break;
  case SET_EPOCH_TIME:
  {
    set_epoch_time();
  }
    break;  
  default:
  {
    debug_matrix_output("Good choice", -1);
  }
    break;
  }
}

/**
 * Setup base time and/or current time.
 */
void setup_menu()
{
  menu_seconds = rtc.now().secondstime();
  debug_matrix_output(">> Setup", -1);
  uint32_t menu_seconds = rtc.now().secondstime();
  uint8_t option = 0;
  do
  {
    set_btn.tick();
    change_mode.tick();
    if (set_btn.hasClicks())
    {
      debug_matrix_output("> Set time", -1);
      option = SET_CURRENT_TIME;
    }
    else if (change_mode.hasClicks())
    {
      debug_matrix_output("> Set EPOCH START", -1);
      option = SET_EPOCH_TIME;
    }
  } while ((rtc.now().secondstime() - menu_seconds) < MENU_THRESSHOLD);
  
  menu_action(option);
}

/**
 * set_btn_action -> setup_menu -> menu_action -> set_current_time/set_epoch_time -> user_input_unix_time
 */
void set_btn_action()
{
  if (set_btn.hasClicks())
  {
    debug_output("set btn click");
    setup_menu();
  }
}

void loop()
{
  change_mode.clear();
  set_btn.clear();
  change_mode.tick();
  set_btn.tick();
  serial_setup();
  button_mode_action();
  set_btn_action();
  display_update();
}
