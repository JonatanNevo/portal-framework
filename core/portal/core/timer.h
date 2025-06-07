//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <chrono>
#include "log.h"

namespace portal
{

/**
 * @brief Encapsulates basic usage of chrono, providing a means to calculate float
 *        durations between time points via function calls.
 */
class Timer
{
public:
    using Seconds = std::ratio<1>;
    using Milliseconds = std::ratio<1, 1000>;
    using Microseconds = std::ratio<1, 1000000>;
    using Nanoseconds = std::ratio<1, 1000000000>;

    // Configure
    using Clock = std::chrono::steady_clock;
    using DefaultResolution = Seconds;

    Timer();
    /**
     * @brief Starts the timer, elapsed() now returns the duration since start()
     */
    void start();

    /**
     * @brief Laps the timer, elapsed() now returns the duration since the last lap()
     */
    void lap();

    /**
     * @brief Stops the timer, elapsed() now returns 0
     * @return The total execution time between `start()` and `stop()`
     */
    template <typename T = DefaultResolution>
    float stop()
    {
        if (!running)
            return 0;

        running = false;
        lapping = false;
        auto duration = std::chrono::duration<float, T>(Clock::now() - start_time);
        start_time = Clock::now();
        lap_time = Clock::now();

        return duration.count();
    }

    /**
     * @brief Calculates the time difference between now and when the timer was started
     *        if lap() was called, then between now and when the timer was last lapped
     * @return The duration between the two time points (default in seconds)
     */
    template <typename T = DefaultResolution>
    float elapsed()
    {
        if (!running)
            return 0;

        Clock::time_point start = start_time;
        if (lapping)
            start = lap_time;

        return std::chrono::duration<float, T>(Clock::now() - start).count();
    }

    /**
     * @brief Calculates the time difference between now and the last time this function was called
     * @return The duration between the two time points (default in seconds)
     */
    template <typename T = DefaultResolution>
    float tick()
    {
        const auto now = Clock::now();
        auto duration = std::chrono::duration<float, T>(now - previous_tick);
        previous_tick = now;
        return duration.count();
    }

    /**
     * @brief Check if the timer is running
     */
    [[nodiscard]] bool is_running() const;

private:
    bool running{false};
    bool lapping{false};
    Clock::time_point start_time;
    Clock::time_point lap_time;
    Clock::time_point previous_tick;
};



template <typename T = Timer::DefaultResolution>
class ScopedTimer
{
public:
    explicit ScopedTimer(const std::string_view& name) :
        name(name) {}

    ~ScopedTimer()
    {
        auto duration = timer.stop<T>();
        LOG_CORE_INFO_TAG("Timer", "\"{}\" - {} s", name, duration);
    }

private:
    std::string_view name;
    Timer timer;
};


} // namespace portal
