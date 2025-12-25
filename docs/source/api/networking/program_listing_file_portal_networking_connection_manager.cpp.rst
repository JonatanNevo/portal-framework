
.. _program_listing_file_portal_networking_connection_manager.cpp:

Program Listing for File connection_manager.cpp
===============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_connection_manager.cpp>` (``portal\networking\connection_manager.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "connection_manager.h"
   
   #include <stdexcept>
   #include <string>
   
   #include <steam/isteamnetworkingutils.h>
   #include <steam/steamnetworkingsockets.h>
   #ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
   #include <steam/steam_api.h>
   #endif
   
   #include "connection.h"
   
   namespace portal::network
   {
   ConnectionManager* ConnectionManager::instance = nullptr;
   std::mutex ConnectionManager::instance_mutex;
   
   ConnectionManager* ConnectionManager::get_instance()
   {
       std::lock_guard<std::mutex> lock(instance_mutex);
       if (instance == nullptr)
           instance = new ConnectionManager();
       return instance;
   }
   
   ConnectionManager::ConnectionManager()
   {
       SteamNetworkingErrMsg error;
       const auto success = GameNetworkingSockets_Init(nullptr, error);
       if (!success)
           throw std::runtime_error("Failed to initialize GameNetworkingSockets: " + std::string(error));
       sockets = SteamNetworkingSockets();
   }
   
   ConnectionManager::~ConnectionManager()
   {
       GameNetworkingSockets_Kill();
   }
   
   void ConnectionManager::add_connection(const HSteamNetConnection connection, Connection* connection_object)
   {
       if (connections.contains(connection))
           throw std::runtime_error("Connection already exists");
       connections[connection] = connection_object;
   }
   
   Connection* ConnectionManager::get_connection(const HSteamNetConnection connection)
   {
       if (!connections.contains(connection))
           return nullptr;
       return connections[connection];
   }
   
   void ConnectionManager::add_server(const HSteamListenSocket server, Server* server_object)
   {
       if (servers.contains(server))
           throw std::runtime_error("Server already exists");
       servers[server] = server_object;
   }
   
   void ConnectionManager::remove_server(const HSteamListenSocket server)
   {
       if (!servers.contains(server))
           return;
       servers.erase(server);
   }
   
   Server* ConnectionManager::get_server(const HSteamListenSocket server)
   {
       if (!servers.contains(server))
           return nullptr;
       return servers[server];
   }
   
   void ConnectionManager::remove_connection(const HSteamNetConnection connection)
   {
       if (!connections.contains(connection))
           return;
       connections.erase(connection);
   }
   } // namespace portal::network
