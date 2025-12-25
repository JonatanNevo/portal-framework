
.. _program_listing_file_portal_core_strings_hash.h:

Program Listing for File hash.h
===============================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_strings_hash.h>` (``portal\core\strings\hash.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <string>
   #include <portal/core/common.h>
   
   #include "rapidhash/rapidhash.h"
   
   # if !defined(PORTAL_COMPILER_MSVC)
   #   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE constexpr
   # else
   #   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE
   # endif
   
   namespace portal::hash
   {
   PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char* data, const size_t length)
   {
       return ::rapidhash(data, length);
   }
   
   PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const std::string_view str)
   {
       return ::rapidhash(str.data(), str.length());
   }
   
   PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const std::string& str)
   {
       return ::rapidhash(str.c_str(), str.length());
   }
   
   PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char* str)
   {
       return ::rapidhash(str, std::strlen(str));
   }
   
   template <size_t n>
   PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char (&data)[n])
   {
       // Exclude null terminator from hash
       return ::rapidhash(data, n - 1);
   }
   }
