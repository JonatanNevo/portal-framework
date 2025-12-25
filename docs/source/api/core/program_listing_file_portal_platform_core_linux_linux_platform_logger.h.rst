
.. _program_listing_file_portal_platform_core_linux_linux_platform_logger.h:

Program Listing for File linux_platform_logger.h
================================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_platform_core_linux_linux_platform_logger.h>` (``portal\platform\core\linux\linux_platform_logger.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <spdlog/spdlog.h>
   
   namespace portal::platform
   {
   const std::vector<spdlog::sink_ptr>& get_platform_sinks();
   
   bool print_assert_dialog(std::string_view file, int line, std::string_view function, std::string_view message);
   }
