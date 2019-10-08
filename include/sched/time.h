#ifndef INCLUDE_TIME_H_
#define INCLUDE_TIME_H_

#include "type.h"

#define FREQUENCY 252000000

#define TIME_SLICE 10 //ms

extern uint32_t time_elapsed;

uint32_t get_timer(void);

uint32_t get_ticks(void);

void latency(uint32_t time);

extern uint32_t timer_increasement;

#endif