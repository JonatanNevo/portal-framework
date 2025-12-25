
.. _program_listing_file_portal_networking_connection.cpp:

Program Listing for File connection.cpp
=======================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_networking_connection.cpp>` (``portal\networking\connection.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   //
   // Copyright © 2025 Jonatan Nevo.
   // Distributed under the MIT license (see LICENSE file).
   //
   
   #include "connection.h"
   #include "connection_manager.h"
   #include "portal/core/log.h"
   
   #include <format>
   #include <steam/isteamnetworkingutils.h>
   #include <steam/steamnetworkingsockets.h>
   #ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
   #include <steam/steam_api.h>
   #endif
   
   #include "portal/core/debug/assert.h"
   #include "portal/networking/utils.h"
   
   namespace portal::network
   {
   Connection::Connection() : manager(ConnectionManager::get_instance()), sockets(manager->get_sockets()) {}
   
   Connection::Connection(ConnectionManager* manager) : manager(manager), sockets(manager->get_sockets()) {}
   
   Connection::~Connection() { disconnect(); }
   
   void Connection::connect(const std::string& address)
   {
       if (running)
       {
           LOG_WARN_TAG("Networking", "Connection - {} is already running", connection);
           return;
       }
   
       SteamNetworkingIPAddr ip_address{};
       if (!is_valid_id_address(address))
           ip_address = resolve_address(address)[0];
   
       SteamNetworkingConfigValue_t options{};
       options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(Connection::on_status_changed_callback));
       connection = sockets->ConnectByIPAddress(ip_address, 1, &options);
       if (connection == k_HSteamNetConnection_Invalid)
       {
           state = ConnectionState::ProblemDetectedLocally;
           return;
       }
       manager->add_connection(connection, this);
   
       running = true;
   
       if (polling_thread.joinable())
           polling_thread.join();
   
       polling_thread = std::thread(&Connection::thread_loop, this);
   }
   
   void Connection::disconnect()
   {
       running = false;
   
       if (polling_thread.joinable())
           polling_thread.join();
   
       for (const auto& callback : on_disconnect_callbacks)
           callback();
   
       sockets->CloseConnection(connection, static_cast<int>(ConnectionEnd::AppConnectionClosed), "Connection closed", true);
       manager->remove_connection(connection);
       connection = k_HSteamNetConnection_Invalid;
   }
   
   void Connection::send_buffer(const Buffer buffer, const bool reliable) { send_raw(buffer.data, buffer.size, reliable); }
   
   void Connection::send_string(const std::string& string, const bool reliable) { send_raw(string.data(), string.size(), reliable); }
   
   void Connection::send_raw(const void* data, const size_t size, const bool reliable)
   {
       const auto flags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
       const auto result = sockets->SendMessageToConnection(connection, data, static_cast<uint32_t>(size), flags, nullptr);
       switch (result)
       {
       case k_EResultOK:
           // everything is ok
           break;
       case k_EResultInvalidParam:
           LOG_ERROR_TAG("Networking", "Connection - {} Invalid connection, cannot send");
           break;
       case k_EResultInvalidState:
           LOG_ERROR_TAG("Networking", "Connection - {} Invalid state, cannot send", connection);
           break;
       case k_EResultNoConnection:
           LOG_ERROR_TAG("Networking", "Connection - {} is already invalid, cannot send", connection);
           break;
       case k_EResultLimitExceeded:
           LOG_ERROR_TAG("Networking", "Connection - {} Limit exceeded, cannot send", connection);
           break;
   
       default:
           LOG_WARN_TAG("Networking", "SendMessageToConnection should not return {}", static_cast<int>(result));
           break;
       }
   }
   
   SteamNetConnectionInfo_t Connection::get_connection_info() const
   {
       SteamNetConnectionInfo_t info{};
       if (sockets->GetConnectionInfo(connection, &info))
           return info;
       LOG_ERROR_TAG("Networking", "Connection - {} Failed to get connection info", connection);
       return info;
   }
   
   void Connection::thread_loop()
   {
       using namespace std::chrono_literals;
   
       while (running)
       {
           poll_incoming_messages();
           poll_connection_state_changes();
       }
   }
   
   void Connection::poll_incoming_messages()
   {
       SteamNetworkingMessage_t* incoming_message = nullptr;
       const int message_count = sockets->ReceiveMessagesOnConnection(connection, &incoming_message, 1);
   
       // No messages to process
       if (message_count == 0)
           return;
   
       if (message_count < 0)
       {
           LOG_ERROR_TAG("Networking", "Connection - {} Failed to receive message", connection);
           running = false;
       }
   
   
       for (const auto& callback : on_data_received_callbacks)
       {
           const auto buffer = Buffer(incoming_message->m_pData, incoming_message->m_cbSize);
           callback(buffer);
       }
   
       incoming_message->Release();
   }
   
   void Connection::poll_connection_state_changes() const { sockets->RunCallbacks(); }
   
   
   void Connection::on_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* info)
   {
       const auto manager = ConnectionManager::get_instance();
       const auto connection = manager->get_connection(info->m_hConn);
       PORTAL_ASSERT(connection != nullptr, "Connection not found in manager");
       if (connection)
           connection->on_status_changed(info);
   }
   
   void Connection::on_status_changed(SteamNetConnectionStatusChangedCallback_t* info)
   {
       switch (info->m_info.m_eState)
       {
       case k_ESteamNetworkingConnectionState_None:
           // Clean up after connection destruction
           LOG_DEBUG_TAG("Networking", "Connection {} - State changed to None", info->m_info.m_szConnectionDescription);
           break;
   
       case k_ESteamNetworkingConnectionState_ClosedByPeer:
           state = ConnectionState::ClosedByPeer;
           if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
               LOG_ERROR_TAG(
               "Networking",
               "Connection {} - Failed to connect: {}",
               info->m_info.m_szConnectionDescription,
               info->m_info.m_szEndDebug
           );
           else
               LOG_ERROR_TAG(
               "Networking",
               "Connection {} - Disconnected from server: {}",
               info->m_info.m_szConnectionDescription,
               info->m_info.m_szEndDebug
           );
           on_connection_error(&info->m_info);
           break;
   
       case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
           state = ConnectionState::ProblemDetectedLocally;
           if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
               LOG_ERROR_TAG(
               "Networking",
               "Connection {} - Failed to connect: {}",
               info->m_info.m_szConnectionDescription,
               info->m_info.m_szEndDebug
           );
           else
               LOG_ERROR_TAG(
               "Networking",
               "Connection {} - Problem detected locally: {}",
               info->m_info.m_szConnectionDescription,
               info->m_info.m_szEndDebug
           );
           on_connection_error(&info->m_info);
           break;
   
       case k_ESteamNetworkingConnectionState_Connecting:
           LOG_DEBUG_TAG("Networking", "Connection {} - State changed to Connecting", info->m_info.m_szConnectionDescription);
           state = ConnectionState::Connecting;
           break;
   
       case k_ESteamNetworkingConnectionState_FindingRoute:
           LOG_DEBUG_TAG("Networking", "Connection {} - State changed to FindingRoute", info->m_info.m_szConnectionDescription);
           state = ConnectionState::FindingRoute;
           break;
   
       case k_ESteamNetworkingConnectionState_Connected:
           char target_str[SteamNetworkingIdentity::k_cchMaxString];
           info->m_info.m_identityRemote.ToString(target_str, sizeof(target_str));
           LOG_INFO_TAG("Networking", "Connection {} - Connected to {}", info->m_info.m_szConnectionDescription, target_str);
           state = ConnectionState::Connected;
           for (const auto& callback : on_connect_callbacks)
               callback();
           break;
   
       default:
           LOG_TRACE_TAG(
               "Networking",
               "Connection {} - State changed to {}",
               info->m_info.m_szConnectionDescription,
               static_cast<int>(info->m_info.m_eState)
           );
           break;
       }
   }
   
   void Connection::on_connection_error(SteamNetConnectionInfo_t* /*info*/)
   {
       running = false;
       for (const auto& callback : on_disconnect_callbacks)
           callback();
   
       sockets->CloseConnection(connection, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Connection error", false);
       manager->remove_connection(connection);
       connection = k_HSteamNetConnection_Invalid;
   }
   
   
   void Connection::on_fatal_error(const std::string& message)
   {
       LOG_ERROR_TAG("Networking", "Fatal error - {}", message);
       running = false;
   }
   } // namespace portal::network
