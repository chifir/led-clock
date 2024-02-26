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
#define BASE_TIMESTAMP_COUNT_OFFSET 0
#define BASE_TIMESTAMP_OFFSET 1
#define BASE_DEFAULT_TIMESTAMP 410455800
#define DEFAULT_TIMEZONE 3

// button settings
#define EB_DEB_TIME 50
#define EB_CLICK_TIME 500
#define EB_HOLD_TIME 600
#define EB_STEP_TIME 200

const uint8_t DISPLAY_WIDTH = 63;
const uint8_t HEIGHT = 3;
const uint8_t WiDITH = 3;
const uint8_t MODE[5] = {2, 8, 10, 16, 0};
const uint8_t MODE_SIZE = 5;
const uint8_t DEBOUNCE_DELAY_MS = 15;
const uint8_t MENU_THRESSHOLD = 5;

enum display_mode
{
  bin = 2,
  oct = 8,
  dec = 10,
  hex = 16,
  str = 0
};

volatile uint32_t seconds = 0;
volatile uint32_t base_timestamp = 0;
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
void display_time(DateTime time_to_display, uint8_t mode)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  switch (mode)
  {
  case display_mode::bin:
  {
    display_bin(seconds);
  }
  break;
  case display_mode::oct:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(seconds, display_mode::oct);
  }
  break;
  case display_mode::dec:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(seconds, display_mode::dec);
  }
  break;
  case display_mode::hex:
  {
    mtrx.setCursor(27, 0);
    mtrx.print(seconds, display_mode::hex);
  }
  break;
  case display_mode::str:
  {
    char date[6] = "hh:mm";
    time_to_display.toString(date);
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
    DateTime now = rtc.now();
    seconds = rtc.now().secondstime() + base_timestamp;
    display_time(now, MODE[CURRENT_MODE_INDEX]);
    trigger_display_update = false;
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

  CURRENT_MODE_INDEX = 1;
}

void display_setup()
{
  mtrx.begin();
  mtrx.clear();
  mtrx.update();
}

byte get_count()
{
  return eeprom_read_byte(BASE_TIMESTAMP_COUNT_OFFSET);
}

uint32_t get_base_timestamp(byte index)
{
  return eeprom_read_dword((const uint32_t *)(1 + 4 * index));
}

void update_base_timestamp(byte index, uint32_t baseTimestamp)
{
  eeprom_update_dword((uint32_t *)(1 + 4 * index), baseTimestamp);
}

/**
 * Writes default base timestamp in EEPROM, if it wasn't set previously.
 * Reads base timestamp from EEPROM.
 */
void setup_eeprom()
{
  Serial.println(get_count());
  Serial.print("#EEPROM default value: ");
  base_timestamp = get_base_timestamp(0);
  Serial.print(base_timestamp);
  if (~base_timestamp == 0)
  {
    debug_output("base wasn't set");
    update_base_timestamp(0, BASE_DEFAULT_TIMESTAMP);
    base_timestamp = get_base_timestamp(0);
  }
}

void setup()
{
  if (DEBUG)
  {
    Serial.begin(9800);
  }

  debug_output("start setup");
  debug_output("display setup");
  display_setup();
  debug_output("rtc setup");
  rtc_setup();
  debug_output("EEPROM setup");
  setup_eeprom();
}

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
int16_t user_inupt(char *msg_template, int16_t min, int16_t max, int16_t current)
{
  debug_output("user input");
  menu_seconds = rtc.now().secondstime();
  char *msg;
  do
  {
    set_btn.tick();
    change_mode.tick();

    msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, msg_template, current);
    debug_matrix_output(msg, -1);

    if (set_btn.hasClicks()) 
    {
      current = current + set_btn.getClicks();
    } else if (change_mode.hasClicks())
    {
      current = current - change_mode.getClicks();
    }

    if (set_btn.hasClicks() || change_mode.hasClicks())
    {
      menu_seconds = rtc.now().secondstime();
    }

    current = check_user_input(min, max, current);

    free(msg);
  } while ((rtc.now().secondstime() - menu_seconds) < MENU_THRESSHOLD);
  free(msg);
  menu_seconds = rtc.now().secondstime();
  return current;
}

/**
 * Converts user input into unixtime object.
 */
UnixTime user_input_unix_time()
{
  DateTime current_date_time = rtc.now();
  debug_output("get_unix_time");
  uint32_t menu_seconds = rtc.now().secondstime();
  do
  {
    int8_t utc_time_zone = (int8_t)user_inupt("TZ GMT(%d)", -11, 12, (int16_t)DEFAULT_TIMEZONE);
    uint16_t year = (uint16_t)user_inupt("YEAR: %d", 1970, 2099, (int16_t)current_date_time.year());
    uint8_t month = (uint8_t)user_inupt("MONTH: %d", 1, 12, (int16_t)current_date_time.month());
    uint8_t max_day = (uint8_t)get_days_in_month(month, year);
    uint8_t day = (uint8_t)user_inupt("DAY: %d", 1, max_day, (int16_t)current_date_time.day());
    uint8_t hour = (uint8_t)user_inupt("HOUR: %d", 0, 23, (int16_t)current_date_time.hour());
    uint8_t minute = (uint8_t)user_inupt("MINUTE: %d", 0, 59, (int16_t)current_date_time.minute());

    UnixTime unix_time(utc_time_zone);
    unix_time.setDateTime(year, month, day, hour, minute, 0);

    return unix_time;
  } while ((rtc.now().secondstime() - menu_seconds) < MENU_THRESSHOLD);

  return NULL;
}

/**
 * Setup base time and/or current time.
 */
void setup_menu()
{
  menu_seconds = rtc.now().secondstime();
  debug_matrix_output(">>> Setup", -1);
  UnixTime user_inupt = user_input_unix_time();
}

/**
 * set_btn_action -> setup_menu -> choose_option -> user_input_unix_time -> user_input
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
  change_mode.tick();
  set_btn.tick();

  button_mode_action();
  set_btn_action();
  display_update();
}
