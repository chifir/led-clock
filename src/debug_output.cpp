#include "debug_output.h"

#define DEBUG false

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
    sprintf(msg, "%lu", num);
    Serial.println(msg);
    free(msg);
  }
}

void debug_output(int32_t num)
{
  if (DEBUG)
  {
    char *msg = (char *)calloc(12, sizeof(char));
    sprintf(msg, "%ld", num);
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

void debug_output_unixtimestamp(UnixStamp unixStamp)
{  
  if (DEBUG)
  {
    civil_time time = unixStamp.getTime();
    char *msg = (char *)calloc(32, sizeof(char));
    sprintf(msg, "%d:%d:%d:%d:%d", time.year, time.mon, time.day, time.hour, time.min);
    Serial.println(msg);
    free(msg);
  }
}
