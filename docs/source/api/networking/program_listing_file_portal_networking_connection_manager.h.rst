
.. _program_listing_file_portal_networking_connection_manager.h:

Program Listing for File connection_manager.h
=============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_connection_manager.h>` (``portal\networking\connection_manager.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #pragma once
   
   #include <map>
   #include <mutex>
   #include <steam/steamnetworkingtypes.h>
   
   class ISteamNetworkingSockets;
   
   namespace portal::network
   {
   class Server;
   class Connection;
   
   class ConnectionManager
   {
   public:
       ConnectionManager(ConnectionManager& other) = delete;
       void operator=(const ConnectionManager&) = delete;
   
       static ConnectionManager* get_instance();
       [[nodiscard]] ISteamNetworkingSockets* get_sockets() const { return sockets; }
   
       void add_connection(HSteamNetConnection connection, Connection* connection_object);
       void remove_connection(HSteamNetConnection connection);
       Connection* get_connection(HSteamNetConnection connection);
   
       void add_server(HSteamListenSocket server, Server* server_object);
       void remove_server(HSteamListenSocket server);
       Server* get_server(HSteamListenSocket server);
   
   protected:
       ConnectionManager();
       ~ConnectionManager();
   
   private:
       static ConnectionManager* instance;
       static std::mutex instance_mutex;
       ISteamNetworkingSockets* sockets;
       std::map<HSteamNetConnection, Connection*> connections;
       std::map<HSteamListenSocket, Server*> servers;
   };
   } // namespace portal::network
