
.. _program_listing_file_portal_core_strings_string_registry.cpp:

Program Listing for File string_registry.cpp
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_strings_string_registry.cpp>` (``portal\core\strings\string_registry.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "string_registry.h"
   
   #include <map>
   #include <memory_resource>
   #include <stdexcept>
   
   namespace portal
   {
   std::string_view StringRegistry::store(uint64_t id, const std::string_view string)
   {
       auto& entries = get_entries();
   
       const auto it = entries.find(id);
       if (it != entries.end())
           return it->second;
   
       // Saves a copy of the string in memory
       const auto& [added_it, success] = entries.emplace(id, std::pmr::string(string, get_allocator()));
   
       if (!success)
           throw std::runtime_error("Failed to store string in StringIdPool");
   
       return std::string_view(added_it->second);
   }
   
   std::string_view StringRegistry::find(const uint64_t id)
   {
       auto entries = get_entries();
       const auto it = entries.find(id);
       if (it != entries.end())
           return it->second;
       return INVALID_STRING_VIEW;
   }
   
   std::pmr::memory_resource* StringRegistry::get_allocator()
   {
       // Initial size of 64k bytes
       static auto buffer_resource = std::pmr::monotonic_buffer_resource{64 * 1024, std::pmr::new_delete_resource()};
       static auto pool_resource = std::pmr::unsynchronized_pool_resource{&buffer_resource};
       return &pool_resource;
   }
   
   std::pmr::unordered_map<uint64_t, std::pmr::string>& StringRegistry::get_entries()
   {
       static std::pmr::unordered_map<uint64_t, std::pmr::string> entries{get_allocator()};
       return entries;
   }
   } // portal
