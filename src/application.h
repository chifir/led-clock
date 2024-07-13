#ifndef APPLICATION_H
#define APPLICATION_H

#include <UnixTime.h>
//#include <EncButton.h>
#include "rtc_clock.h"
#include "debug_output.h"
#include "storage.h"
#include "matrix_display.h"
#include "user_input.h"

#define EPOCH_BEGIN 536229000
#define EPOCH_BEGIN_OFFSET 0
#define DEFAULT_TIMEZONE 3
#define CLOCK_OFFSET 1
// menu
#define SET_CURRENT_TIME 1
#define SET_EPOCH_TIME 2

#define CLOCK_INTERRUPT_PIN 2


const uint8_t MODE[5] = {2, 8, 10, 16, 0};
const uint8_t MODE_SIZE = 5;

extern uint8_t CURRENT_MODE_INDEX;
extern bool trigger_display_update;
extern int8_t current_timezone;
extern uint32_t epoch_begin_timestamp;
extern uint32_t clock_timestamp;



void setup_app();

void run_app();

#endif