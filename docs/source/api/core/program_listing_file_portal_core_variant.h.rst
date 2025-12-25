
.. _program_listing_file_portal_core_variant.h:

Program Listing for File variant.h
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_variant.h>` (``portal\core\variant.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <variant>
   
   namespace portal
   {
   namespace details
   {
       template <class... Ts>
       struct visitor : Ts...
       {
           using Ts::operator()...;
       };
   }
   
   template <typename Variant, class... Matchers>
   decltype(auto) match(Variant&& variant, Matchers&&... matchers)
   {
       return std::visit(details::visitor<Matchers>(matchers)..., std::forward<Variant>(variant));
   }
   }
