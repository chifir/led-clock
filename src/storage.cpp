#include "storage.h"

/**
 * Reads GMT from eeprom
*/
int8_t get_timezone()
{
  eeprom_busy_wait();
  return (int8_t) eeprom_read_byte(EEPROM_GMT_OFFSET);
}

/**
 * Writes GMT to eeprom
*/
void update_gmt(int8_t gmt) 
{
  eeprom_busy_wait();
  eeprom_update_byte(EEPROM_GMT_OFFSET, gmt);
}

/**
 * Reads timestamp(4 bytes) from eeprom by index:
 * 0 - base timestamp
 * 1 - clock timestamp (just for recovering)
*/
uint32_t get_eeprom_timestamp(byte index)
{
  eeprom_busy_wait();
  return eeprom_read_dword((const uint32_t *)(EEPROM_TIMESTAMP_OFFSET + sizeof(uint32_t) * index));
}

/**
 * Wtites timestamp(4 bytes) to eeprom by index:
 * 0 - base timestamp
 * 1 - clock timestamp
*/
void update_eeprom_timestamp(byte index, uint32_t baseTimestamp)
{
  eeprom_busy_wait();
  eeprom_update_dword((uint32_t *)(EEPROM_TIMESTAMP_OFFSET + sizeof(uint32_t) * index), baseTimestamp);
}
