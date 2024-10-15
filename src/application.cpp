#include "application.h"

#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_CHOOSE_PIN 7
#define BUTTON_SETTINGS_PIN 8

Button mode_btn(BUTTON_CHANGE_MODE_PIN, INPUT_PULLUP, LOW);
Button choose_btn(BUTTON_CHOOSE_PIN, INPUT_PULLUP, LOW);
Button settings_btn(BUTTON_SETTINGS_PIN, INPUT_PULLUP, LOW);

uint8_t CURRENT_MODE_INDEX = 0;
bool trigger_display_update = true;
int8_t current_timezone = 0;
int8_t epoch_timezone = DEFAULT_TIMEZONE;
uint32_t epoch_begin_timestamp = 0;
uint32_t clock_timestamp = 0;

/**
 * Update display info.
 */
void update_display()
{
  if (trigger_display_update)
  {
    trigger_display_update = false;
    UnixStamp unix_time(rtc.now().unixtime(), current_timezone);
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
void mode_action(Button *btn)
{
  if (btn->hasClicks())
  {
    debug_output("mode_action");
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

void edit_current_time() 
{
  debug_output("edit_current_time start");
  // convert to unix
  DateTime now = rtc.now();
  civil_time current_time = UnixStamp::convertUnixToTime(now.unixtime(), current_timezone);
  UnixStamp user_time = user_input_time(current_time, current_timezone, rtc, choose_btn, mode_btn);
  // update rtc clock 
  DateTime time(user_time.getUnix());
  rtc.adjust(time);
  // udpate eeprom for recovery
  update_eeprom_timestamp(CLOCK_OFFSET, user_time.getUnix());

  debug_dislpay_timestamps();
  debug_output("edit_current_time end");
}

void edit_epoch()
{
  debug_output("edit_epoch");
  debug_dislpay_timestamps();
  uint32_t eeprom_epoch = get_eeprom_timestamp(EPOCH_BEGIN_OFFSET);
  UnixStamp src_epoch_stamp(eeprom_epoch, current_timezone);
  debug_output("before");
  debug_output_unixtimestamp(src_epoch_stamp.getUnix());
  // get time from user
  UnixStamp user_input_epoch = user_input_time(src_epoch_stamp.getTime(), current_timezone, rtc, choose_btn, mode_btn);
  debug_output("after");
  debug_output_unixtimestamp(user_input_epoch.getUnix());
  // udpate eeprom for recovery
  update_eeprom_timestamp(EPOCH_BEGIN_OFFSET, user_input_epoch.getUnix());
  debug_output("READ from EEPROM after update");
  debug_dislpay_timestamps();
  debug_output("###");
}

void menu_action(uint8_t option)
{
  debug_output("menu_action");
  debug_output(option);

  switch (option)
  {
  case SET_CURRENT_TIME:
  {
    edit_current_time();
  }
    break;
  case SET_EPOCH_TIME:
  {
    edit_epoch();
  }
    break;  
  default:
  {
    debug_matrix_output("Good choice", -1);
  }
    break;
  }
}

void display_edit_time() 
{
  char date[17] = "YYYY/MM/DD hh:mm";
  rtc.now().toString(date);
  matrix_display_string(date);
}

void display_edit_epoch()
{
  uint32_t epoch = get_eeprom_timestamp(EPOCH_BEGIN_OFFSET);
  civil_time epoch_civil = UnixStamp::convertUnixToTime(epoch, 0);
  char *date = (char *)calloc(17, sizeof(char));
  sprintf(date, "%d/%d/%d %d:%d", epoch_civil.year, epoch_civil.mon, epoch_civil.day, epoch_civil.hour, epoch_civil.min);  
  matrix_display_string(date);
  free(date);
}

/**
 * Setup base time and/or current time.
 */
uint8_t choose_option(RTC_DS3231 *rtc, Button *choose_btn, Button *settings_btn)
{
  uint32_t menu_seconds = 0;
  bool should_display_edit_time = true;
  menu_seconds = rtc->now().secondstime();
  do
  {
    choose_btn->clear();
    settings_btn->clear();
    choose_btn->tick();
    settings_btn->tick();

    if (should_display_edit_time) 
    {
      display_edit_time();
      if (choose_btn->hasClicks())
      {
        return SET_CURRENT_TIME;
      }
    } 
    else
    {
      display_edit_epoch();
      if (choose_btn->hasClicks())
      {
        return SET_EPOCH_TIME;
      }
    }

    if (settings_btn->hasClicks()) 
    {
      should_display_edit_time = !should_display_edit_time;
      menu_seconds = rtc->now().secondstime();
    }
  } while ((rtc->now().secondstime() - menu_seconds) < MENU_THRESSHOLD);
  
  return NO_ACTION;
}

/**
 * settings_action -> choose_option -> menu_action -> edit_current_time/edit_current_time -> user_input_time
 */
void settings_action(RTC_DS3231 *rtc, Button *choose_btn, Button *settings_btn)
{
  if (settings_btn->hasClicks())
  {
    debug_output("settings_action");
    uint8_t option = choose_option(rtc, choose_btn, settings_btn);
    if (option == NO_ACTION) {
      debug_output("settings_action:NO_ACTION");
      return;
    }
    menu_action(option);
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
  mode_btn.clear();
  choose_btn.clear();
  mode_btn.tick();
  choose_btn.tick();
  settings_btn.clear();
  settings_btn.tick();

  mode_action(&mode_btn);

  settings_action(&rtc, &choose_btn, &settings_btn);
  
  update_display();
}
