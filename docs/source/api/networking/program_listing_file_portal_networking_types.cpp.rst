
.. _program_listing_file_portal_networking_types.cpp:

Program Listing for File types.cpp
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_types.cpp>` (``portal\networking\types.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "types.h"
   
   
   namespace portal::network::internal
   {
   inline ConnectionState from_steam_networking_state(ESteamNetworkingConnectionState state)
   {
       switch (state)
       {
       case k_ESteamNetworkingConnectionState_Connecting:
           return ConnectionState::Connecting;
       case k_ESteamNetworkingConnectionState_FindingRoute:
           return ConnectionState::FindingRoute;
       case k_ESteamNetworkingConnectionState_Connected:
           return ConnectionState::Connected;
       case k_ESteamNetworkingConnectionState_ClosedByPeer:
           return ConnectionState::ClosedByPeer;
       case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
           return ConnectionState::ProblemDetectedLocally;
       default:
           return ConnectionState::None;
       }
   }
   } // namespace portal::network::internal
