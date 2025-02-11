//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <chrono>
#include <iostream>

namespace portal
{

class Timer
{
public:
    Timer() { reset(); }
    void reset() { start = std::chrono::high_resolution_clock::now(); }

    float elapsed() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() * 0.001f * 0.001f *
            0.001f;
    }

    float elapsed_ms() const { return elapsed() * 1000.0f; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

class ScopedTimer
{
public:
    ScopedTimer(const std::string_view& name) :
        name(name) {}

    ~ScopedTimer()
    {
        const float time = timer.elapsed_ms();
        std::cout << "Timer " << name << " took " << time << " ms" << std::endl;
    }

private:
    std::string_view name;
    Timer timer;
};


} // namespace portal
