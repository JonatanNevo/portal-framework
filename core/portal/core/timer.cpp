//
// Created by Jonatan Nevo on 01/03/2025.
//

#include "timer.h"

namespace portal
{
Timer::Timer(): start_time{Clock::now()}, previous_tick{Clock::now()}
{
}

void Timer::start()
{
    if (!running)
    {
        running = true;
        start_time = Clock::now();
    }
}

void Timer::lap()
{
    lapping = true;
    lap_time = Clock::now();
}

bool Timer::is_running() const
{
    return running;
}
}
