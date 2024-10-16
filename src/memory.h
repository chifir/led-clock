#ifndef NENORY_H
#define NENORY_H

#include <stdint.h>
#include <string.h>
#include "debug_output.h"

extern uint32_t __heap_start, *__brkval;

uint32_t getFreeMemorySize(); 
void debug_output_free_memory();
void debug_output_free_memory(const char *label);

#endif