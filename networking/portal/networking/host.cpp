//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "host.h"

#include <ranges>
#include <steam/steamnetworkingsockets.h>

#include "portal/networking/connection_manager.h"
#include "portal/networking/types.h"
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace portal::network
{
Host::Host(const int port) :
    port(port), manager(ConnectionManager::get_instance()), sockets(manager->get_sockets()) {}

Host::Host(ConnectionManager* manager, int port) :
    port(port), manager(manager), sockets(manager->get_sockets()) {}

Host::~Host()
{
    stop();
    if (polling_thread.joinable())
        polling_thread.join();
}

void Host::start()
{
    if (running)
    {
        LOG_CORE_WARN_TAG("Networking", "Host - {} is already running", port);
        return;
    }

    SteamNetworkingIPAddr local_address;
    local_address.Clear();
    local_address.m_port = port;

    SteamNetworkingConfigValue_t options{};
    options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(Host::on_status_changed_callback));

    listen_socket = sockets->CreateListenSocketIP(local_address, 1, &options);
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
    manager->add_host(listen_socket, this);
    polling_thread = std::thread(&Host::thread_loop, this);
}

void Host::stop()
{
    running = false;

    if (polling_thread.joinable())
        polling_thread.join();

    LOG_CORE_INFO_TAG("Networking", "Host {} - Closing connections", port);
    for (const auto& id : connections | std::views::keys)
    {
        sockets->CloseConnection(id, static_cast<int>(ConnectionEnd::AppConnectionClosed), "Host closing", false);
    }

    connections.clear();

    sockets->CloseListenSocket(listen_socket);
    listen_socket = k_HSteamListenSocket_Invalid;

    sockets->DestroyPollGroup(poll_group);
    poll_group = k_HSteamNetPollGroup_Invalid;
}

void Host::thread_loop()
{
    using namespace std::chrono_literals;

    LOG_CORE_INFO_TAG("Networking", "Host {} - Starting to host", port);
    while (running)
    {
        poll_incoming_messages();
        poll_connection_state_changes();
        std::this_thread::sleep_for(10ms);
    }
}

void Host::on_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* info)
{
    const auto host = ConnectionManager::get_instance()->get_host(info->m_info.m_hListenSocket);
    PORTAL_CORE_ASSERT(host != nullptr, "Host not found in manager");
    if (host)
        host->on_status_changed(info);
}

void Host::on_status_changed(SteamNetConnectionStatusChangedCallback_t* info)
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
            PORTAL_CORE_ASSERT(connection != connections.end(), "Client not found in host");

            LOG_CORE_INFO_TAG("Networking", "Host {} - Connection closed: {}", port, connection->second.connection_description);

            for (const auto& callback : on_connection_disconnect_callbacks)
                callback(connection->second);

            connections.erase(connection);
        }
        sockets->CloseConnection(info->m_hConn, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Error in connection", false);
        break;

    case k_ESteamNetworkingConnectionState_Connecting:
    {
        // This must be a new connection
        PORTAL_CORE_ASSERT(!connections.contains(info->m_hConn), "Client already exists in host");

        if (sockets->AcceptConnection(info->m_hConn) != k_EResultOK)
        {
            LOG_CORE_ERROR_TAG("Networking", "Host {} - Failed to accept connection: {}", port, info->m_info.m_szConnectionDescription);
            sockets->CloseConnection(info->m_hConn, static_cast<int>(ConnectionEnd::AppExceptionGeneric), "Failed to accept connection", false);
            break;
        }

        if (!sockets->SetConnectionPollGroup(info->m_hConn, poll_group))
        {
            LOG_CORE_ERROR_TAG(
                "Networking",
                "Host {} - Failed to set poll group for connection: {}",
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

        LOG_CORE_INFO_TAG("Networking", "Host {} - New connection: {}", port, connection.connection_description);
        for (const auto& callback : on_connection_connect_callbacks)
            callback(connection);
        break;
    }
    default:
        break;
    }
}


void Host::send_buffer(const HSteamNetConnection id, const Buffer buffer, const bool reliable) const
{
    send_raw(id, buffer.data, buffer.size, reliable);
}

void Host::send_buffer_to_all(const Buffer buffer, const HSteamNetConnection exclude, const bool reliable)
{
    send_raw_to_all(buffer.data, buffer.size, exclude, reliable);
}

void Host::send_string(const HSteamNetConnection id, const std::string& string, const bool reliable) const
{
    send_raw(id, string.data(), string.size(), reliable);
}

void Host::send_string_to_all(const std::string& string, const HSteamNetConnection exclude, const bool reliable)
{
    send_raw_to_all(string.data(), string.size(), exclude, reliable);
}

void Host::send_raw(HSteamNetConnection id, const void* data, const size_t size, const bool reliable) const
{
    const auto flags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
    const auto result = sockets->SendMessageToConnection(id, data, size, flags, nullptr);
    switch (result)
    {
    case k_EResultOK:
        // everything is ok
        break;
    case k_EResultInvalidParam:
        LOG_CORE_ERROR_TAG("Networking", "Connection - {} Invalid connection, cannot send");
        break;
    case k_EResultInvalidState:
        LOG_CORE_ERROR_TAG("Networking", "Connection - {} Invalid state, cannot send", id);
        break;
    case k_EResultNoConnection:
        LOG_CORE_ERROR_TAG("Networking", "Connection - {} is already invalid, cannot send", id);
        break;
    case k_EResultLimitExceeded:
        LOG_CORE_ERROR_TAG("Networking", "Connection - {} Limit exceeded, cannot send", id);
        break;

    default:
        LOG_CORE_WARN_TAG("Networking", "SendMessageToConnection should not return {}", static_cast<int>(result));
        break;
    }
}

void Host::send_raw_to_all(const void* data, size_t size, HSteamNetConnection exclude, bool reliable)
{
    for (const auto& key : connections | std::views::keys)
    {
        if (key == exclude)
            continue;
        send_raw(key, data, size, reliable);
    }
}

void Host::kick_client(const HSteamNetConnection id) const
{
    sockets->CloseConnection(id, static_cast<int>(ConnectionEnd::AppKickedByHost), "Kicked by host", false);
}

void Host::poll_incoming_messages()
{
    ISteamNetworkingMessage* incoming_message = nullptr;
    const int message_count = sockets->ReceiveMessagesOnPollGroup(poll_group, &incoming_message, 1);
    if (message_count == 0)
        return;

    if (message_count < 0)
    {
        LOG_CORE_ERROR_TAG("Networking", "Host - {} Failed to receive message", port);
        running = false;
        return;
    }

    const auto client = connections.find(incoming_message->m_conn);
    if (client == connections.end())
    {
        LOG_CORE_ERROR_TAG("Networking", "Host - {} Client not found", port);
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

void Host::poll_connection_state_changes() const { sockets->RunCallbacks(); }
void Host::set_client_nick(const HSteamNetConnection id, const char* nick) const { sockets->SetConnectionName(id, nick); }
void Host::on_fatal_error(const std::string& message) {}

} // namespace portal::network
