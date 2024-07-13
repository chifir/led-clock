#include "application.h"

#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_SET_PIN 7

Button change_mode(BUTTON_CHANGE_MODE_PIN, INPUT_PULLUP, LOW);
Button set_btn(BUTTON_SET_PIN, INPUT_PULLUP, LOW);

uint8_t CURRENT_MODE_INDEX = 0;
bool trigger_display_update = true;
int8_t current_timezone = 0;
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
    display_time(unix_time_to_epoch_time(unix_time, epoch_begin_timestamp), MODE[CURRENT_MODE_INDEX]);
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
}

// TODO: remove debug func
void dislpay_timestamps() 
{
  debug_output("EPOCH:");
  Serial.println(get_eeprom_timestamp(EPOCH_BEGIN_OFFSET));
  debug_output("CLOCK");
  Serial.println(get_eeprom_timestamp(CLOCK_OFFSET));
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
  // udpate eeprom for recovery
  update_eeprom_timestamp(CLOCK_OFFSET, user_date.timestamp);
  // update rtc clock 
  rtc.adjust(date_data_to_date_time(user_date));

  dislpay_timestamps();
}

void user_updates_epoch()
{
  dislpay_timestamps();
  UnixTime epoch_start(current_timezone);
  epoch_start.getDateTime(get_eeprom_timestamp(EPOCH_BEGIN_OFFSET));
  debug_output("before");
  debug_output_unixtimestamp(epoch_start);
  // get time from user
  DateData user_epoch_start = user_input_unix_time(epoch_start, current_timezone, rtc, set_btn, change_mode);
  debug_output("after");
  Serial.println(user_epoch_start.timestamp);
  debug_output_unixtimestamp(user_epoch_start.timestamp);
  // udpate eeprom for recovery
  update_eeprom_timestamp(EPOCH_BEGIN_OFFSET, user_epoch_start.timestamp);
  dislpay_timestamps();
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
    user_updates_epoch();
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
  dislpay_timestamps();
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
