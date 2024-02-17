#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>
#include <GyverMAX7219.h>
#include <GyverButton.h>
#include <avr/eeprom.h>

#define DEBUG true
#define CLOCK_INTERRUPT_PIN 2
#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_SET_PIN 7
#define BASE_TIMESTAMP_COUNT_OFFSET 0
#define BASE_TIMESTAMP_OFFSET 1
#define BASE_DEFAULT_TIMESTAMP 410455800

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

uint8_t CURRENT_MODE_INDEX = 0;
uint32_t offset = 0;
u8 deltas_count = 0;

// SDA on A4, SCL on A5, SQW on D4
RTC_DS3231 rtc;
RTC_I2C rtc_i2c;

// 8 matrix in 1 row on D5
MAX7219<12, 1, 5> mtrx;

GButton change_mode(BUTTON_CHANGE_MODE_PIN);
GButton set_btn(BUTTON_SET_PIN);

void debug_output(const char *msg)
{
  if (DEBUG) Serial.println(msg);
}

void debug_output(int num)
{
  if (DEBUG) {
    char *msg = (char*)calloc(12, sizeof(char));
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
  if (delay != -1) _delay_ms(delay);
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
 * Setup a button.
*/
void button_setup(GButton button,  uint16_t debounce, uint16_t timeout, uint16_t click_timeout, bool direction, bool type)
{
  button.setDebounce(50);
  button.setTimeout(300);
  button.setClickTimeout(600);
  button.setDirection(NORM_OPEN);
  button.setType(HIGH_PULL);
}

/**
 * Setup buttons.
 */
void buttons_setup()
{
  button_setup(change_mode, 50, 300, 6000, NORM_OPEN, HIGH_PULL);
  button_setup(set_btn, 50, 300, 6000, NORM_OPEN, HIGH_PULL);
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

byte get_count() {
  return eeprom_read_byte(BASE_TIMESTAMP_COUNT_OFFSET);
}

uint32_t get_base_timestamp(byte index) {
  return eeprom_read_dword((const uint32_t *)(1 + 4 * index));
}

void update_base_timestamp(byte index, uint32_t baseTimestamp) {
  eeprom_update_dword((uint32_t *)(1 + 4 * index), baseTimestamp);
}

/**
 * Writes default base timestamp in EEPROM, if it wasn't set previously.
 * Reads base timestamp from EEPROM.
*/
void setup_eeprom() {
  Serial.println(get_count());
  Serial.print("#EEPROM default value: ");
  base_timestamp = get_base_timestamp(0);
  Serial.print(base_timestamp);
  if (~base_timestamp == 0) {
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
  debug_output("button setup");
  buttons_setup();
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

int8_t check_user_input(int8_t min, int8_t max, int8_t input) {
  return (input < min) ? min : ((input > max ) ? max : input);
}

int8_t user_inupt(char *msg_template, int8_t min, int8_t max, int8_t current)
{
  debug_output("user input");
  uint32_t menu_seconds = rtc.now().secondstime();
  char *msg;
  do
  {
    set_btn.resetStates();
    change_mode.resetStates();

    set_btn.tick();
    change_mode.tick();

    msg = (char*)calloc(12, sizeof(char));
    sprintf(msg, msg_template, current);
    debug_matrix_output(msg, -1);

    if (set_btn.hasClicks()) {
      current++;
    }

    if (change_mode.hasClicks()) {
      current--;
    }

    current = check_user_input(min, max, current);
    if (set_btn.hasClicks() || change_mode.hasClicks()) {
      menu_seconds = rtc.now().secondstime();
    }

    free(msg);
  } while ((rtc.now().secondstime() - menu_seconds) < MENU_THRESSHOLD);
  free(msg);

  return current;
}

/**
 * Setup base time and/or current time.
*/
void setup_menu(){
  uint8_t timezone =  user_inupt("TZ UTC(%d)", -11, 12, 3);
  char *msg = (char*)calloc(10, sizeof(char));
  debug_output("TimeZone:");
  debug_output(timezone);
  // menu_seconds = rtc.now().secondstime();
  // debug_matrix_output(">>> Setup", -1);
  // do
  // {
  //   if (set_btn.hasClicks()) {
  //     menu_seconds = rtc.now().secondstime();
  //     // setup current time
  //     debug_output("current time");
  //     debug_matrix_output("Set current date", 3000);
  //     DateTime current_datetime = rtc.now();
  //     rtc.now().year 

  //   } else if (change_mode.hasClicks()){
  //     // setup begining
  //     debug_output("begining");
  //   }
  // } while ((menu_seconds - rtc.now().secondstime()) < MENU_THRESSHOLD);
}

/**
 * set_btn_action -> setup_menu -> user-input
*/
void set_btn_action() {
  if (set_btn.isClick()) {
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
