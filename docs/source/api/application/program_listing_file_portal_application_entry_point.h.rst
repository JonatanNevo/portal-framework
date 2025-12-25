
.. _program_listing_file_portal_application_entry_point.h:

Program Listing for File entry_point.h
======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_application_entry_point.h>` (``portal\application\entry_point.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   
   std::unique_ptr<portal::Application> portal::create_application(int argc, char** argv) {
       ApplicationProperties props{.name = STRING_ID("My Game")};
       return std::make_unique<MyGameApp>(props);
   }
   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   
   #pragma once
   
   #include <portal/application/application.h>
   
   extern std::unique_ptr<portal::Application> portal::create_application(int arc, char** argv);
   
   namespace portal
   {
   int main(int argc, char** argv)
   {
       // TODO: fetch default log levels from environment?
       Log::init({.default_log_level = Log::LogLevel::Trace});
   
       auto application = create_application(argc, argv);
       try
       {
           application->run();
       }
       catch (std::exception& e)
       {
           LOG_FATAL("Unhandled exception: {}", e.what());
       }
       catch (...)
       {
           LOG_FATAL("Unhandled unknown exception");
       }
       application.reset();
   
       Log::shutdown();
       return 0;
   }
   }
   
   #if defined(PORTAL_PLATFORM_WINDOWS) && defined(PORTAL_DIST)
   
   #include <Windows.h>
   
   int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
   {
       return portal::main(__argc, __argv);
   }
   
   #else
   
   int main(int argc, char** argv)
   {
       return portal::main(argc, argv);
   }
   
   #endif
