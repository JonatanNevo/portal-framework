
.. _program_listing_file_portal_networking_utils.h:

Program Listing for File utils.h
================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_utils.h>` (``portal\networking\utils.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   #include <string_view>
   #include <vector>
   #include <steam/steamnetworkingtypes.h>
   
   
   namespace portal::network
   {
   bool is_valid_id_address(std::string_view ip);
   std::vector<SteamNetworkingIPAddr> resolve_address(std::string_view address);
   }
