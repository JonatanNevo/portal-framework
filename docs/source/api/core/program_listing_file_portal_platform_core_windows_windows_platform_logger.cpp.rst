
.. _program_listing_file_portal_platform_core_windows_windows_platform_logger.cpp:

Program Listing for File windows_platform_logger.cpp
====================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_platform_core_windows_windows_platform_logger.cpp>` (``portal\platform\core\windows\windows_platform_logger.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "windows_platform_logger.h"
   
   #include <map>
   #include <spdlog/sinks/basic_file_sink.h>
   #include <spdlog/sinks/stdout_color_sinks.h>
   #include <spdlog/sinks/msvc_sink.h>
   
   #include <windows.h>
   
   #define PORTAL_HAS_CONSOLE !PORTAL_DIST
   
   namespace portal::platform
   {
   const std::vector<spdlog::sink_ptr>& get_platform_sinks()
   {
       static std::vector<spdlog::sink_ptr> sinks;
       if (sinks.empty())
       {
           sinks = {
               std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/portal.log", true),
   #if defined(PORTAL_DEBUG) || defined(PORTAL_DEVELOPMENT)
               std::make_shared<spdlog::sinks::msvc_sink_mt>(),
   #endif
   #if PORTAL_HAS_CONSOLE
               std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
   #endif
           };
       }
   
       return sinks;
   }
   
   bool print_assert_dialog(std::string_view file, int line, std::string_view function, std::string_view message)
   {
       volatile static bool do_assert = true;
   
       typedef std::pair<int, std::string> AssertLocation;
       static std::map<AssertLocation, bool> assertion_map;
   
       const AssertLocation assert_location = std::make_pair(line, std::string(file));
       if (assertion_map[assert_location])
           return false;
   
       if (do_assert && IsDebuggerPresent())
       {
           int res = IDTRYAGAIN;
           EndMenu();
           {
               std::thread assert_dialog(
                   [&]()
                   {
                       res = MessageBoxA(
                           nullptr,
                           std::format(
                               "Assert failed at:\n{}({})\n{}()\n{}\nTry again to debug, Cancel to ignore this assert in the future",
                               file,
                               line,
                               function,
                               message
                           ).c_str(),
                           "ASSERTION",
                           MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_TOPMOST
                       );
                   }
               );
               assert_dialog.join();
           }
   
           if (res == IDCANCEL)
               // ignore this assert in the future
               assertion_map[assert_location] = true;
           else if (res == IDTRYAGAIN)
               return true; // break at outer context only if got option to select IDCANCEL
       }
   
       return false;
   }
   }
