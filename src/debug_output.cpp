#include "debug_output.h"

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

void debug_output_unixtimestamp(UnixTime unix_time)
{  
  char *msg = (char *)calloc(32, sizeof(char));
  sprintf(msg, "%d:%d:%d:%d:%d", unix_time.year, unix_time.month, unix_time.day, unix_time.hour, unix_time.minute);
  Serial.println(msg);
  free(msg);
}
