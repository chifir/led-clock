#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <avr/eeprom.h>
#include "debug_output.h"
#include <stdint.h>

#define EEPROM_GMT_OFFSET 0
#define EEPROM_TIMESTAMP_OFFSET 1

int8_t get_timezone();

void update_gmt(int8_t gmt);

uint32_t get_eeprom_timestamp(byte index);

void update_eeprom_timestamp(byte index, uint32_t baseTimestamp);

#endif