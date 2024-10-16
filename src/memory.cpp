#include "memory.h"

uint32_t getFreeMemorySize()
{
    unsigned int v;
    return (unsigned int)&v - (__brkval == 0 ? (unsigned int)&__heap_start : (unsigned int)__brkval);
}

void debug_output_free_memory(const char *label)
{
    char *msg = (char *)calloc(strlen(label) + 5, sizeof(char));
    sprintf(msg, "%s: %lu", label, getFreeMemorySize());
    debug_output(msg);
    free(msg);
}

void debug_output_free_memory()
{
    debug_output(getFreeMemorySize());
}
