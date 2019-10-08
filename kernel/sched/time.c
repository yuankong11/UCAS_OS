#include "time.h"
#include "debug.h"

uint32_t time_elapsed = 0;

uint32_t timer_increasement = TIME_SLICE * (FREQUENCY / 2000); //avoid overflow

uint32_t get_ticks()
{
    return time_elapsed;
}

uint32_t get_timer()
{
    return time_elapsed * TIME_SLICE; //ms
}

void latency(uint32_t time)
{
    uint32_t begin_time = get_timer();
    while (get_timer() - begin_time < time)
        ;
}