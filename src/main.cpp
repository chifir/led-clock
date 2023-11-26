#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>
#include <GyverMAX7219.h>

#define DEBUG true
#define CLOCK_INTERRUPT_PIN 2
#define MODE_BUTTON_PIN 6

const uint64_t DELTA = 410455800;

volatile uint32_t seconds = 0;

// SDA on A4, SCL on A5, SQW on D4
RTC_DS3231 rtc; 
RTC_I2C rtc_i2c;

// 4 matrix in 1 row on D5
MAX7219 <4 , 1, 5> mtrx; 

void debug_output(const char* msg) {
  Serial.println(msg);
}

void button_mode_handler() {
  // todo
}

/**
 * SQW signal interruption handler
 * 
 * - reads current date from RTC
 * - sends data to the display handler
*/
void rtc_interruption_handler() {
  seconds++;
  Serial.println(seconds);
  Serial.println(seconds, 2);
  debug_output("SQW_INT");
}

void button_setup() {
  // assign interruption handler
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), rtc_interruption_handler, FALLING);
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
}

void display_setup() {
  mtrx.begin();
  mtrx.clear();
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
  button_setup();
  debug_output("rtc setup");
  rtc_setup();
  seconds = rtc.now().secondstime() + DELTA;
}

void print_datetime() {
    char date[14] = "YYYY:hh:mm:ss";
    rtc.now().toString(date);
    Serial.println(date);
}

void loop() {
  mtrx.clear();
  mtrx.setCursor(0, 0);
  // mtrx.print(seconds);
  char date[6] = "hh:mm";
  rtc.now().toString(date);
  mtrx.print(date);
  mtrx.update(); 
}
