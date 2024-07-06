#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <HardwareSerial.h>

#define DEBUG true

void debug_output(const char *msg)
{
  if (DEBUG)
    Serial.println(msg);
}

void debug_output(uint32_t num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_output(uint16_t num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(20, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_output(int num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, "%d", num);
    Serial.println(msg);
    free(msg);
  }
}

#endif
