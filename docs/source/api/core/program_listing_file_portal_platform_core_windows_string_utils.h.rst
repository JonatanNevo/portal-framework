
.. _program_listing_file_portal_platform_core_windows_string_utils.h:

Program Listing for File string_utils.h
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_platform_core_windows_string_utils.h>` (``portal\platform\core\windows\string_utils.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <string>
   
   namespace portal::details
   {
   enum class Encoding
   {
       UTF8,
       ANSI
   };
   
   std::wstring to_wstring(const std::string& str, Encoding encoding = Encoding::UTF8);
   }
