#include <SPI.h>
#include <Arduino.h>
#include <RTClib.h>

RTC_DS3231 clock; 
DateTime time;

void setup() {
  Serial.begin(57600);
  
  if (!clock.begin()){
    Serial.println("Couldn't start I2C for ds3221");
    Serial.flush();
    while (1) delay(10);
  }

  if (clock.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    clock.adjust(DateTime(2023, 1, 21, 3, 0, 0));
  }
}

void loop() {
  time = clock.now();
  Serial.println(time.unixtime());
}
