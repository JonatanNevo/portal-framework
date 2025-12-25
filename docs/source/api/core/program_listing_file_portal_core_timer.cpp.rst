
.. _program_listing_file_portal_core_timer.cpp:

Program Listing for File timer.cpp
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_timer.cpp>` (``portal\core\timer.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "timer.h"
   
   namespace portal
   {
   SpinLock PerformanceProfiler::lock{};
   
   Timer::Timer() : start_time{Clock::now()}, previous_tick{Clock::now()}
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
   
   void PerformanceProfiler::set_frame_timing(const char* name, float sampled_time)
   {
       std::scoped_lock guard{lock};
       if (!data.contains(name))
           data[name] = {.time = 0.f, .samples = 0};
   
       auto& [time, samples] = data[name];
       time += sampled_time;
       samples++;
   }
   
   void PerformanceProfiler::clear()
   {
       std::scoped_lock guard{lock};
       data.clear();
   }
   
   ScopedPerformanceTimer::ScopedPerformanceTimer(const char* name, PerformanceProfiler* profiler) : name(name), profiler(profiler)
   {
       timer.start();
   }
   
   ScopedPerformanceTimer::~ScopedPerformanceTimer()
   {
       const float time = timer.stop<Timer::DefaultResolution>();
       profiler->set_frame_timing(name, time);
   }
   }
