#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>
#include <GyverMAX7219.h>

#define DEBUG true
#define CLOCK_INTERRUPT_PIN 2
#define BUTTON_CHANGE_MODE_PIN 6
#define BUTTON_CHANGE_UNIT_PIN 7

const uint8_t DISPLAY_WIDTH = 63;
const uint8_t HEIGHT = 3;
const uint8_t WiDITH = 3;
const uint8_t MODE[5] = {2, 8, 10, 16, 0};
const uint8_t MODE_SIZE = 5;
const uint8_t DEBOUNCE_DELAY_MS = 50;

enum display_mode{
  bin = 2,
  oct = 8,
  dec = 10,
  hex = 16,
  str = 0
};

const uint64_t DELTA = 410455800;

volatile uint32_t seconds = 0;

volatile bool trigger_display_update = true;

uint8_t CURRENT_MODE_INDEX = 0;

uint32_t offset = 0;

bool final_button_state = LOW;
bool current_button_state = LOW;
bool previous_button_state = LOW;
uint8_t debounce_time_ms = 0;

// SDA on A4, SCL on A5, SQW on D4
RTC_DS3231 rtc; 
RTC_I2C rtc_i2c;

// 8 matrix in 1 row on D5
MAX7219 <12 , 1, 5> mtrx; 

void debug_output(const char* msg) {
  Serial.println(msg);
}

void debug_matrix_output(char* msg, double delay){
  mtrx.clear();
  mtrx.setCursor(0, 0);
  mtrx.print(msg);
  mtrx.update();
  _delay_ms(delay);
}

void graph_sin(uint8_t offset) {
  uint8_t x = 0;
  uint8_t y = 0;
  mtrx.clear();
  mtrx.setCursor(0, 0);
  for(uint8_t i = 0 + offset; i <= 95 + offset; i++) {
    y = 4 * (1 + sinf(25.7 * i));
    mtrx.dot(x, y);
    x++;
  }
  mtrx.update();
}

void display_bin(uint32_t time) {
  bool bin_time[32];
  uint8_t x = 0;
  uint8_t y = 0;

  for(uint8_t i = 0; i < sizeof(uint32_t) * 8; i++) 
  {
    uint8_t num = (time >> i) & 1; 
    bin_time[sizeof(uint32_t) * 8 - 1 - i] = num == 1;
  }

  for(uint8_t i = 0; i < sizeof(uint32_t) * 8; i++) 
  {
    bool tmp_bool = bin_time[i];
    if (tmp_bool) {
      mtrx.lineV(x, y, y + HEIGHT - 1);
    } else {
      mtrx.rectWH(x, y, WiDITH, HEIGHT, GFX_STROKE);
    }
    x = x + WiDITH + 1;
    if (x >= DISPLAY_WIDTH) {
      x = 0;
      y = HEIGHT + 1;
    }
  }
}

void display_time(DateTime time_to_display, uint8_t mode) {
  mtrx.clear();
  mtrx.setCursor(0, 0);
  switch (mode)
  {
  case display_mode::bin: {
    display_bin(seconds);
  }
    break;
  case display_mode::oct: {
    mtrx.print(seconds, display_mode::oct);
  }
    break;
  case display_mode::dec: {
    mtrx.print(seconds, display_mode::dec);
  }
    break;
  case display_mode::hex: {
    mtrx.print(seconds, display_mode::hex);
  }
    break;
  case display_mode::str:{
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
void display_update() {
  if (trigger_display_update) {
    DateTime now = rtc.now();
    seconds = now.secondstime() + DELTA;
    display_time(now, MODE[CURRENT_MODE_INDEX]);
    trigger_display_update = false;
  }
}

void display_format_mode_change() {
  if (CURRENT_MODE_INDEX >= 0 && CURRENT_MODE_INDEX < MODE_SIZE - 1) {
    CURRENT_MODE_INDEX++;
  } else if (CURRENT_MODE_INDEX == MODE_SIZE - 1) {
    CURRENT_MODE_INDEX = 0;
  }
}

/**
 * SQW signal interruption handler
 * 
 * - reads current date from RTC
 * - sends data to the display handler
*/
void rtc_interruption_handler() {
  trigger_display_update = true;
}

void buttons_setup() {
  pinMode(BUTTON_CHANGE_MODE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CHANGE_UNIT_PIN, INPUT_PULLUP);
}

/**
 * Setup DS3231:
 * - connect
 * - setup time if power lost
 * - clean alarm registers
 * - set 1Hz on SQW pin
 * - assign 1Hz interruption handler
*/
void rtc_setup(){
  // connect via I2C to the ds3221
  while (!rtc.begin()){
    debug_output("Couldn't connect to the ds3221");
    Serial.flush();
    _delay_ms(10);
  }
  
  // setup compile time if there were powered off
  if (rtc.lostPower()) {
    debug_output("RTC lost power, setup compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    debug_output("RTC hasnt lost power");
  }
  
  //we don't need the 32K Pin, so disable it
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

void display_setup() {
  mtrx.begin();
  mtrx.clear();
  mtrx.update();
}

void test_binary() {
  int x = 0;
  int y = 0;

  mtrx.clear();

  for (int i = 0; i < 4; i++) {
    mtrx.rectWH(x, y, 3, 4, GFX_STROKE);
    x = x + 4;
    y = 0;
    mtrx.lineV(x, y, y + 3);
    x = x + 4;
    y = 0;
  }

  mtrx.update();
}

void setup() {
  debug_output("start setup");
  if (DEBUG) {
    Serial.begin(9800);
  }

  debug_output("display setup");
  display_setup();
  debug_output("button setup");
  buttons_setup();
  debug_output("rtc setup");
  rtc_setup();
  seconds = rtc.now().secondstime() + DELTA;
}

void print_datetime() {
    char date[14] = "YYYY:hh:mm:ss";
    rtc.now().toString(date);
    Serial.println(date);
}

bool button_mode_read(){
  int current_button_state = digitalRead(BUTTON_CHANGE_MODE_PIN);
  if (current_button_state != previous_button_state) {
    debounce_time_ms = millis();
  }
  if ((millis() - debounce_time_ms) >= DEBOUNCE_DELAY_MS) 
  {
    final_button_state = digitalRead(BUTTON_CHANGE_MODE_PIN);
  }
  previous_button_state = current_button_state;
  return final_button_state;
}


void loop() {
  display_update();
  if (button_mode_read()) {
    display_format_mode_change();
  }
}
