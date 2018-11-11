#include "time.h"

uint32_t time_elapsed = 0;

uint32_t get_ticks()
{
    return time_elapsed;
}

uint32_t get_timer()
{
    return time_elapsed / MHZ; // in microseconds
}

void latency(uint32_t time)
{
    uint32_t begin_time = get_timer();

    while (get_timer() - begin_time < time)
    {
    };
    return;
}
