#include "application.h"

#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_SET_PIN 7

Button change_mode(BUTTON_CHANGE_MODE_PIN, INPUT_PULLUP, LOW);
Button set_btn(BUTTON_SET_PIN, INPUT_PULLUP, LOW);

uint8_t CURRENT_MODE_INDEX = 0;
bool trigger_display_update = true;
int8_t current_timezone = 0;
int8_t epoch_timezone = DEFAULT_TIMEZONE;
uint32_t epoch_begin_timestamp = 0;
uint32_t clock_timestamp = 0;

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
    display_time(unix_time_to_epoch_time(unix_time, epoch_begin_timestamp), MODE[CURRENT_MODE_INDEX], rtc);
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

// TODO: remove debug func
void debug_dislpay_timestamps() 
{
  debug_output("EPOCH:");
  Serial.println(get_eeprom_timestamp(EPOCH_BEGIN_OFFSET));
  debug_output("CLOCK");
  Serial.println(get_eeprom_timestamp(CLOCK_OFFSET));
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
  clock_timestamp = get_eeprom_timestamp(CLOCK_OFFSET);
  // read and set epoch begining timestamp
  epoch_begin_timestamp = get_eeprom_timestamp(EPOCH_BEGIN_OFFSET);
  if (~epoch_begin_timestamp == 0)
  {
    debug_output("begining wasn't set");
    update_eeprom_timestamp(0, EPOCH_BEGIN);
    epoch_begin_timestamp = EPOCH_BEGIN;
  }
  debug_output("####");
  debug_dislpay_timestamps();
}

void user_updates_current_time() 
{
  debug_output("set_current_time start");
  // convert to unix
  debug_output("get rtc.now() for converting into ");
  DateTime now = rtc.now();
  debug_output("convert to unix");
  UnixTime current_time = date_time_to_unix_time(current_timezone, now);
  // get time from user
  // current_time = user_input_unix_time(current_time, current_timezone);
  DateData user_date = user_input_unix_time(current_time, current_timezone, rtc, set_btn, change_mode);
  // update rtc clock 
  rtc.adjust(date_data_to_date_time(user_date));
  // udpate eeprom for recovery
  update_eeprom_timestamp(CLOCK_OFFSET, user_date.timestamp);

  debug_dislpay_timestamps();
}

void user_updates_epoch()
{
  debug_output("###");
  debug_dislpay_timestamps();
  UnixTime epoch_start(current_timezone);
  uint32_t eeprom_epoch = get_eeprom_timestamp(EPOCH_BEGIN_OFFSET);
  debug_output("EPOCH from EEPROM more human readable");
  epoch_start.getDateTime(eeprom_epoch);
  debug_output_unixtimestamp(epoch_start);
  debug_output("before what we got by reading from EEPROM");
  debug_output("EPOCH from EEPROM");
  debug_output(eeprom_epoch);
  // get time from user
  DateData user_epoch_start = user_input_unix_time(epoch_start, current_timezone, rtc, set_btn, change_mode);
  debug_output("after");
  debug_output_unixtimestamp(user_epoch_start.timestamp);
  // udpate eeprom for recovery
  update_eeprom_timestamp(EPOCH_BEGIN_OFFSET, user_epoch_start.timestamp);
  debug_output("READ from EEPROM after update");
  debug_dislpay_timestamps();
  debug_output("###");
}

void serial_print(const char* msg, uint64_t num) {
    char *str = (char *) calloc(32, sizeof(char));
    sprintf(str, "%s: %u", msg, num);
    Serial.println(str);
    free(str);
}

void splitUnix(uint32_t unix_time, int8_t _gmt) {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t dayOfWeek;

    unix_time += _gmt * 3600ul;
    second = unix_time % 60ul;
    unix_time /= 60ul;
    minute = unix_time % 60ul;
    unix_time /= 60ul;
    hour = unix_time % 24ul;
    unix_time /= 24ul;
    dayOfWeek = (unix_time + 4) % 7;
    if (!dayOfWeek) dayOfWeek = 7;
    serial_print("unix_time", unix_time);
    uint64_t z = unix_time + 719468;
    serial_print("z", z);
    uint8_t era = z / 146097ul;
    serial_print("era", era);
    uint32_t doe = z - era * 146097ul;
    uint16_t doe_16 = z - era * 146097ul;
    uint32_t doe_32 = z - era * 146097ul;
    serial_print("doe_16", doe_16);
    serial_print("doe_32", doe_32);
    uint16_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint16_t y = yoe + era * 400;
    uint16_t doy = doe - (yoe * 365 + yoe / 4 - yoe / 100);
    uint16_t mp = (doy * 5 + 2) / 153;
    day = doy - (mp * 153 + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : -9);
    y += (month <= 2);
    year = y;

  char *msg = (char *)calloc(32, sizeof(char));
  sprintf(msg, "%d:%d:%d:%d:%d", year, month, day, hour, minute);
  Serial.println(msg);
}

void user_updates_epoch_v2()
{
  debug_output("user_updates_epoch_v2 start");
  debug_dislpay_timestamps();
  UnixTime epoch_unix_time(epoch_timezone);
  epoch_unix_time.getDateTime(epoch_begin_timestamp);
  DateTime dateTime(epoch_begin_timestamp);
  debug_output("date algo: ");
  splitUnix(epoch_begin_timestamp, 3);
  splitUnix(1725792638, 3);

  debug_output("date input using Unix");

  UnixTime unix_time(3);
  unix_time.setDateTime(2024, 12, 29, 8, 30, 0);
  char *msg = (char *)calloc(32, sizeof(char));
  sprintf(msg, "%d:%d:%d:%d:%d", unix_time.year, unix_time.month, unix_time.day, unix_time.hour, unix_time.minute);
  Serial.println(msg);
  Serial.println(unix_time.getUnix());
  free(msg);
  debug_output("date input using Stamp");
  Stamp input_stamp(1986, 12, 29, 8, 30, 0);
  *msg = (char *)calloc(32, sizeof(char));
  sprintf(msg, "%d:%d:%d:%d:%d", input_stamp.year(), input_stamp.month(), input_stamp.day(), input_stamp.hour(), input_stamp.minute());
  Serial.println(msg);
  free(msg);
  

  debug_output("date using Stamp");
  Stamp stamp(epoch_begin_timestamp);
  *msg = (char *)calloc(32, sizeof(char));
  sprintf(msg, "%d:%d:%d:%d:%d", stamp.year(), stamp.month(), stamp.day(), stamp.hour(), stamp.minute());
  Serial.println(msg);
  free(msg);
  debug_output_unixtimestamp(epoch_unix_time);
  debug_output("user_updates_epoch_v2 end");
}

void menu_action(uint8_t option)
{
  switch (option)
  {
  case SET_CURRENT_TIME:
  {
    user_updates_current_time();
  }
    break;
  case SET_EPOCH_TIME:
  {
    // user_updates_epoch();
    user_updates_epoch_v2();
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
 * set_btn_action -> setup_menu -> menu_action -> user_updates_current_time/user_updates_current_time -> user_input_unix_time
 */
void set_btn_action()
{
  if (set_btn.hasClicks())
  {
    debug_output("set btn click");
    setup_menu();
  }
}

void setup_clock_interruption() {
  // assign interruption handler
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), rtc_interruption_handler, FALLING);
}

void setup_interruptions() {
  setup_clock_interruption();
}

void setup_app(){
  Serial.begin(9800);
  trigger_display_update = true;
  CURRENT_MODE_INDEX = 0;
  debug_output("EEPROM setup");
  setup_from_eeprom();
  debug_output("start setup");
  debug_output("display setup");
  display_setup();
  debug_output("rtc setup");
  rtc_setup();
  debug_output("setup interruptions");
  setup_interruptions();
  debug_dislpay_timestamps();
  CURRENT_MODE_INDEX = 4;
}

void run_app() {
  change_mode.clear();
  set_btn.clear();
  change_mode.tick();
  set_btn.tick();
  button_mode_action();
  set_btn_action();
  display_update();
}
