#include "matrix_display.h"

enum display_mode
{
  bin = 2,
  oct = 8,
  dec = 10,
  hex = 16,
  str = 0
};

const uint8_t HEIGHT = 3;
const uint8_t WiDITH = 3;
const uint8_t DISPLAY_WIDTH = 63;

// 12 matrix in 1 row on D5
MAX7219<12, 1, 5> mtrx;

void display_setup()
{
  mtrx.begin();
  mtrx.clear();
  mtrx.update();
}

void debug_matrix_output(char *msg, double delay)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  mtrx.print(msg);
  mtrx.update();
  if (delay != -1)
    _delay_ms(delay);
}

void debug_matrix_output(String msg, double delay)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  mtrx.print(msg);
  mtrx.update();
  if (delay != -1)
    _delay_ms(delay);
}

/**
 * Just for fun, writes a sinux graph on the display
*/
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
void display_time(uint32_t time_to_display, uint8_t mode, RTC_DS3231 rtc)
{
  mtrx.clear();
  mtrx.setCursor(0, 0);
  switch (mode)
  {
  case display_mode::bin:
  {
    display_bin(time_to_display);
  }
  break;
  case display_mode::oct:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(time_to_display, display_mode::oct);
  }
  break;
  case display_mode::dec:
  {
    mtrx.setCursor(16, 0);
    mtrx.print(time_to_display, display_mode::dec);
  }
  break;
  case display_mode::hex:
  {
    mtrx.setCursor(27, 0);
    mtrx.print(time_to_display, display_mode::hex);
  }
  break;
  // only for dev and debug
  case display_mode::str:
  {
    char date[17] = "YYYY:MM:DD:hh:mm";
    rtc.now().toString(date);
    mtrx.print(date);
  }
  break;
  default:
    break;
  }
  mtrx.update();
}
