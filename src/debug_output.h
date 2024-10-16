#ifndef DEBUG_OUTPUT_H
#define DEBUG_OUTPUT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <UnixStamp.hpp>

void debug_output(const char *msg);

void debug_output(uint32_t num);

void debug_output(uint16_t num);

void debug_output(int num);

void debug_output_unixtimestamp(UnixStamp unixstamp);

#endif