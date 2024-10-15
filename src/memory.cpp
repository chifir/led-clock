#include "memory.h"

uint32_t getFreeMemorySize()
{
    unsigned int v;
    return (unsigned int)&v - (__brkval == 0 ? (unsigned int)&__heap_start : (unsigned int)__brkval);
}

void debug_output_free_memory() {
    debug_output("free memory:");
    debug_output(getFreeMemorySize());
}