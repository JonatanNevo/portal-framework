//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <functional>
#include <steam/steamnetworkingtypes.h>
#include <string>

#include "portal/core/buffer.h"


class ISteamNetworkingSockets;

namespace portal::network
{
class ConnectionManager;

struct ConnectionInfo
{
    HSteamNetConnection id;
    std::string connection_description;
};


class Server
{
public:
    Server(int port);
    Server(ConnectionManager* manager, int port);
    ~Server();

    void start();
    void stop();

    // Callback registration
    void register_on_connection_connect_callback(const std::function<void(const ConnectionInfo&)>& callback)
    {
        on_connection_connect_callbacks.push_back(callback);
    }

    void register_on_connection_disconnect_callback(const std::function<void(const ConnectionInfo&)>& callback)
    {
        on_connection_disconnect_callbacks.push_back(callback);
    }

    void register_on_data_received_callback(const std::function<void(const ConnectionInfo&, const Buffer)>& callback)
    {
        on_data_received_callbacks.push_back(callback);
    }


    // Send Data
    void send_buffer(HSteamNetConnection id, Buffer buffer, bool reliable = true) const;
    void send_buffer_to_all(Buffer buffer, HSteamNetConnection exclude = 0, bool reliable = true);
    void send_string(HSteamNetConnection id, const std::string& string, bool reliable = true) const;
    void send_string_to_all(const std::string& string, HSteamNetConnection exclude = 0, bool reliable = true);
    void send_raw(HSteamNetConnection id, const void* data, size_t size, bool reliable = true) const;
    void send_raw_to_all(const void* data, size_t size, HSteamNetConnection exclude = 0, bool reliable = true);

    template <typename T>
    void send_data(const HSteamNetConnection id, const T& data, const bool reliable = true)
    {
        send_raw(id, static_cast<const void*>(&data), sizeof(T), reliable);
    }

    template <typename T>
    void send_data_to_all(const T& data, const HSteamNetConnection exclude = 0, const bool reliable = true)
    {
        send_raw_to_all(static_cast<const void*>(&data), sizeof(T), exclude, reliable);
    }

    // Admin
    void kick_client(HSteamNetConnection id) const;

    // Debugging
    bool is_running() const { return running; }
    const std::map<HSteamNetConnection, ConnectionInfo>& get_connections() const { return connections; }

private:
    void thread_loop();

    static void on_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* info);
    void on_status_changed(SteamNetConnectionStatusChangedCallback_t* info);

    void poll_incoming_messages();
    void set_client_nick(HSteamNetConnection id, const char* nick) const;
    void poll_connection_state_changes() const;

    void on_fatal_error(const std::string& message);

private:
    ConnectionManager* manager;
    ISteamNetworkingSockets* sockets;

    std::vector<std::function<void(const ConnectionInfo&)>> on_connection_connect_callbacks{};
    std::vector<std::function<void(const ConnectionInfo&)>> on_connection_disconnect_callbacks{};
    std::vector<std::function<void(const ConnectionInfo&, Buffer)>> on_data_received_callbacks{};

    int port = 0;
    bool running = false;
    std::map<HSteamNetConnection, ConnectionInfo> connections;
    std::thread polling_thread;

    HSteamListenSocket listen_socket = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup poll_group = k_HSteamNetPollGroup_Invalid;
};

} // namespace portal::network
