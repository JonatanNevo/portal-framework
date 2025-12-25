
.. _program_listing_file_portal_core_timer.h:

Program Listing for File timer.h
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_timer.h>` (``portal\core\timer.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <chrono>
   #include "log.h"
   #include "portal/core/concurrency/spin_lock.h"
   
   namespace portal
   {
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
       void start();
   
       void lap();
   
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
   
       template <typename T = DefaultResolution>
       float tick()
       {
           const auto now = Clock::now();
           auto duration = std::chrono::duration<float, T>(now - previous_tick);
           previous_tick = now;
           return duration.count();
       }
   
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
           LOG_INFO_TAG("Timer", "\"{}\" - {} s", name, duration);
       }
   
   private:
       std::string_view name;
       Timer timer;
   };
   
   class PerformanceProfiler
   {
   public:
       struct PerFrameData
       {
           float time = 0.f;
           uint32_t samples = 0;
       };
   
   public:
       void set_frame_timing(const char* name, float time);
       void clear();
   
   private:
       std::unordered_map<const char*, PerFrameData> data;
       static SpinLock lock;
   };
   
   class ScopedPerformanceTimer
   {
   public:
       ScopedPerformanceTimer(const char* name, PerformanceProfiler* profiler);
       ~ScopedPerformanceTimer();
   
   private:
       const char* name;
       PerformanceProfiler* profiler;
       Timer timer;
   };
   
   #define SCOPED_TIMER(name) ScopedTimer<Timer::DefaultResolution> timer__LINE__(name)
   } // namespace portal
