
.. _program_listing_file_portal_networking_server.cpp:

Program Listing for File server.cpp
===================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_server.cpp>` (``portal\networking\server.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "server.h"
   
   #include <ranges>
   #include <steam/steamnetworkingsockets.h>
   
   #include "portal/networking/connection_manager.h"
   #include "portal/networking/types.h"
   #ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
   #include <steam/steam_api.h>
   #endif
   
   namespace portal::network
   {
   Server::Server(const int port) :
       manager(ConnectionManager::get_instance()),
       sockets(manager->get_sockets()),
       port(port) {}
   
   Server::Server(ConnectionManager* manager, const int port) :
       manager(manager),
       sockets(manager->get_sockets()),
       port(port) {}
   
   Server::~Server()
   {
       stop();
       if (polling_thread.joinable())
           polling_thread.join();
   }
   
   void Server::start()
   {
       if (running)
       {
           LOG_WARN_TAG("Networking", "Server - {} is already running", port);
           return;
       }
   
       SteamNetworkingIPAddr local_address;
       local_address.Clear();
       local_address.m_port = static_cast<uint16_t>(port);
   
       SteamNetworkingConfigValue_t options[1];
       options[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(Server::on_status_changed_callback));
   
       listen_socket = sockets->CreateListenSocketIP(local_address, 1, options);
       if (listen_socket == k_HSteamListenSocket_Invalid)
       {
           on_fatal_error("Failed to create listen socket");
           return;
       }
   
       poll_group = sockets->CreatePollGroup();
       if (poll_group == k_HSteamNetPollGroup_Invalid)
       {
           on_fatal_error("Failed to create poll group");
           return;
       }
   
       running = true;
       manager->add_server(listen_socket, this);
       polling_thread = std::thread(&Server::thread_loop, this);
   }
   
   void Server::stop()
   {
       running = false;
   
       if (polling_thread.joinable())
           polling_thread.join();
   
       LOG_INFO_TAG("Networking", "Server {} - Closing connections", port);
       for (const auto& id : connections | std::views::keys)
       {
           sockets->CloseConnection(id, static_cast<int>(ConnectionEnd::AppConnectionClosed), "Server closing", false);
       }
   
       connections.clear();
   
       sockets->CloseListenSocket(listen_socket);
       listen_socket = k_HSteamListenSocket_Invalid;
   
       sockets->DestroyPollGroup(poll_group);
       poll_group = k_HSteamNetPollGroup_Invalid;
   }
   
   void Server::thread_loop()
   {
       using namespace std::chrono_literals;
   
       LOG_INFO_TAG("Networking", "Server {} - Starting to host", port);
       while (running)
       {
           poll_incoming_messages();
           poll_connection_state_changes();
       }
   }
   
   void Server::on_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* info)
   {
       const auto server = ConnectionManager::get_instance()->get_server(info->m_info.m_hListenSocket);
       PORTAL_ASSERT(server != nullptr, "Server not found in manager");
       if (server)
           server->on_status_changed(info);
   }
   
   void Server::on_status_changed(SteamNetConnectionStatusChangedCallback_t* info)
   {
       switch (info->m_info.m_eState)
       {
       case k_ESteamNetworkingConnectionState_None:
           break;
   
       case k_ESteamNetworkingConnectionState_ClosedByPeer:
       case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
           // Ignore if they were not connected previously
           if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
           {
               // Locate the client
               const auto& connection = connections.find(info->m_hConn);
               PORTAL_ASSERT(connection != connections.end(), "Client not found in server");
   
               LOG_INFO_TAG("Networking", "Server {} - Connection closed: {}", port, connection->second.connection_description);
   
               for (const auto& callback : on_connection_disconnect_callbacks)
                   callback(connection->second);
   
               connections.erase(connection);
           }
           sockets->CloseConnection(info->m_hConn, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Error in connection", false);
           break;
   
       case k_ESteamNetworkingConnectionState_Connecting:
           {
               // This must be a new connection
               PORTAL_ASSERT(!connections.contains(info->m_hConn), "Client already exists in server");
   
               if (sockets->AcceptConnection(info->m_hConn) != k_EResultOK)
               {
                   LOG_ERROR_TAG("Networking", "Server {} - Failed to accept connection: {}", port, info->m_info.m_szConnectionDescription);
                   sockets->CloseConnection(info->m_hConn, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Failed to accept connection", false);
                   break;
               }
   
               if (!sockets->SetConnectionPollGroup(info->m_hConn, poll_group))
               {
                   LOG_ERROR_TAG(
                       "Networking",
                       "Server {} - Failed to set poll group for connection: {}",
                       port,
                       info->m_info.m_szConnectionDescription
                   );
                   sockets->CloseConnection(info->m_hConn, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Failed to set poll group", false);
                   break;
               }
   
               SteamNetConnectionInfo_t connection_info;
               sockets->GetConnectionInfo(info->m_hConn, &connection_info);
   
               ConnectionInfo connection{
                   .id = info->m_hConn,
                   .connection_description = connection_info.m_szConnectionDescription
               };
               connections[info->m_hConn] = connection;
   
               LOG_INFO_TAG("Networking", "Server {} - New connection: {}", port, connection.connection_description);
               for (const auto& callback : on_connection_connect_callbacks)
                   callback(connection);
               break;
           }
       default:
           break;
       }
   }
   
   
   void Server::send_buffer(const HSteamNetConnection id, const Buffer buffer, const bool reliable) const
   {
       send_raw(id, buffer.data, buffer.size, reliable);
   }
   
   void Server::send_buffer_to_all(const Buffer buffer, const HSteamNetConnection exclude, const bool reliable)
   {
       send_raw_to_all(buffer.data, buffer.size, exclude, reliable);
   }
   
   void Server::send_string(const HSteamNetConnection id, const std::string& string, const bool reliable) const
   {
       send_raw(id, string.data(), string.size(), reliable);
   }
   
   void Server::send_string_to_all(const std::string& string, const HSteamNetConnection exclude, const bool reliable)
   {
       send_raw_to_all(string.data(), string.size(), exclude, reliable);
   }
   
   void Server::send_raw(HSteamNetConnection id, const void* data, const size_t size, const bool reliable) const
   {
       const auto flags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
       const auto result = sockets->SendMessageToConnection(id, data, static_cast<uint32_t>(size), flags, nullptr);
       switch (result)
       {
       case k_EResultOK:
           // everything is ok
           break;
       case k_EResultInvalidParam:
           LOG_ERROR_TAG("Networking", "Connection - {} Invalid connection, cannot send");
           break;
       case k_EResultInvalidState:
           LOG_ERROR_TAG("Networking", "Connection - {} Invalid state, cannot send", id);
           break;
       case k_EResultNoConnection:
           LOG_ERROR_TAG("Networking", "Connection - {} is already invalid, cannot send", id);
           break;
       case k_EResultLimitExceeded:
           LOG_ERROR_TAG("Networking", "Connection - {} Limit exceeded, cannot send", id);
           break;
   
       default:
           LOG_WARN_TAG("Networking", "SendMessageToConnection should not return {}", static_cast<int>(result));
           break;
       }
   }
   
   void Server::send_raw_to_all(const void* data, size_t size, HSteamNetConnection exclude, bool reliable)
   {
       for (const auto& key : connections | std::views::keys)
       {
           if (key == exclude)
               continue;
           send_raw(key, data, size, reliable);
       }
   }
   
   void Server::kick_client(const HSteamNetConnection id) const
   {
       sockets->CloseConnection(id, static_cast<int>(ConnectionEnd::AppKickedByServer), "Kicked by server", false);
   }
   
   void Server::poll_incoming_messages()
   {
       ISteamNetworkingMessage* incoming_message = nullptr;
       const int message_count = sockets->ReceiveMessagesOnPollGroup(poll_group, &incoming_message, 1);
       if (message_count == 0)
           return;
   
       if (message_count < 0)
       {
           LOG_ERROR_TAG("Networking", "Server - {} Failed to receive message", port);
           running = false;
           return;
       }
   
       const auto client = connections.find(incoming_message->m_conn);
       if (client == connections.end())
       {
           LOG_ERROR_TAG("Networking", "Server - {} Client not found", port);
           incoming_message->Release();
           return;
       }
   
       if (incoming_message->m_cbSize)
       {
           for (const auto& callback : on_data_received_callbacks)
           {
               const auto buffer = Buffer(incoming_message->m_pData, incoming_message->m_cbSize);
               callback(client->second, buffer);
           }
       }
   
       incoming_message->Release();
   }
   
   void Server::poll_connection_state_changes() const { sockets->RunCallbacks(); }
   void Server::set_client_nick(const HSteamNetConnection id, const char* nick) const { sockets->SetConnectionName(id, nick); }
   void Server::on_fatal_error(const std::string& /*message*/) {}
   } // namespace portal::network
