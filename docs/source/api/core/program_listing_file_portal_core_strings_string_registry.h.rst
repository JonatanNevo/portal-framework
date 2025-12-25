
.. _program_listing_file_portal_core_strings_string_registry.h:

Program Listing for File string_registry.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_strings_string_registry.h>` (``portal\core\strings\string_registry.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <memory_resource>
   #include <string>
   #include <string_view>
   #include <unordered_map>
   
   #include <frozen/unordered_map.h>
   
   
   namespace portal
   {
   constexpr auto INVALID_STRING_VIEW = std::string_view("Invalid");
   
   class StringRegistry
   {
   public:
       static std::string_view store(uint64_t id, std::string_view string);
   
       static std::string_view find(uint64_t id);
   
   private:
       static std::pmr::memory_resource* get_allocator();
       static std::pmr::unordered_map<uint64_t, std::pmr::string>& get_entries();
   };
   } // portal
