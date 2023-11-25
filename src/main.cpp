#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>
#include <GyverMAX7219.h>

#define DEBUG true
#define CLOCK_INTERRUPT_PIN 4
#define MODE_BUTTON_PIN 6

enum dispalay_mode{
  DECIMAL = 0,
  BINARY = 1,
  OCTAL = 2,
  TEXT = 3
};

// target date
TimeSpan target_date = TimeSpan(10000);

// display mode by default
int8_t DISPLAY_MODE = dispalay_mode::DECIMAL;
// SDA on D3, SCL on D2, SQW on D4
RTC_DS3231 rtc; 
// 12 matrix in 1 row on D5
MAX7219 <12 , 1, 5> mtrx; 

DateTime time;


void display_string(const char* msg) {
  mtrx.println(msg);
  mtrx.update();
}

void display_decimal(uint32_t decmal){
  mtrx.println(decmal);
  mtrx.update();
}



void debug_output(const char* msg) {
  Serial.println(msg);
}

/**
 * Displays diff between some point in the past with the current time
*/
void non_stop_watch(DateTime diff) {

}

/**
 * Dispalays date in a specified format
*/
void display_handler(DateTime current_time) {
  DateTime diff = current_time - target_date;
  uint32_t seconds = diff.secondstime();

  switch(DISPLAY_MODE) {
    case dispalay_mode::DECIMAL: {
      debug_output("go to display decimal");
      display_decimal(seconds);    
    }; break;
    case dispalay_mode::BINARY: {}; break;
    case dispalay_mode::OCTAL: {}; break;
    case dispalay_mode::TEXT: {}; break;
  }
}


void button_mode_handler() {
  int8_t DISPLAY_MODE = dispalay_mode::DECIMAL;
}

/**
 * SQW signal interruption handler
 * 
 * - reads current date from RTC
 * - sends data to the display handler
*/
void rtc_interruption_handler() {
  debug_output("SQW interruption handler");
  time = rtc.now();
  display_handler(time);
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
  if (!rtc.begin()){
    debug_output("Couldn't connect to the ds3221");
    Serial.flush();
    while (1) delay(10);
  }
  // setup compile time if there were powered off
  if (rtc.lostPower()) {
    debug_output("RTC lost power, setup compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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

void dispay_setup() {
  mtrx.begin();
  delay(3000);
}

void setup() {
  if (DEBUG) {
    Serial.begin(57600);
  }

  rtc_setup();
}

void loop() {
}
