//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <functional>
#include <steam/steamnetworkingtypes.h>
#include <string>

#include "portal/core/buffer.h"
#include "types.h"

class ISteamNetworkingSockets;
struct SteamNetConnectionStatusChangedCallback_t;
struct SteamNetConnectionInfo_t;


namespace portal::network
{
class ConnectionManager;

class Connection
{
public:
    // Callback registration
    void register_on_connect_callback(const std::function<void()>& callback) { on_connect_callbacks.push_back(callback); }
    void register_on_disconnect_callback(const std::function<void()>& callback) { on_disconnect_callbacks.push_back(callback); }
    void register_on_data_received_callback(const std::function<void(const Buffer)>& callback) { on_data_received_callbacks.push_back(callback); }

    // Constructors
    Connection();
    explicit Connection(ConnectionManager* manager);
    ~Connection();

    // Client Methods
    void connect(const std::string& address);
    void disconnect();

    // Data sending
    void send_buffer(Buffer buffer, bool reliable = true);
    void send_string(const std::string& string, bool reliable = true);
    void send_raw(const void* data, size_t size, bool reliable = true);

    template <typename T>
    void send_data(const T& data, const bool reliable = true)
    {
        send_raw(static_cast<const void*>(&data), sizeof(T), reliable);
    }

    // Debugging
    [[nodiscard]] bool is_running() const { return running; }
    [[nodiscard]] ConnectionState get_state() const { return state; }
    [[nodiscard]] SteamNetConnectionInfo_t get_connection_info() const;

private:
    void thread_loop();
    void poll_incoming_messages();
    void poll_connection_state_changes() const;

    // Static Callbacks
    static void on_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* info);
    void on_status_changed(SteamNetConnectionStatusChangedCallback_t* info);
    void on_connection_error(SteamNetConnectionInfo_t* info);
    void on_fatal_error(const std::string& message);

private:
    ConnectionManager* manager;
    ISteamNetworkingSockets* sockets;

    std::vector<std::function<void()>> on_connect_callbacks{};
    std::vector<std::function<void()>> on_disconnect_callbacks{};
    std::vector<std::function<void(Buffer)>> on_data_received_callbacks{};

    ConnectionState state = ConnectionState::None;
    HSteamNetConnection connection = k_HSteamNetConnection_Invalid;
    std::thread polling_thread;
    bool running = false;
};

} // namespace portal::network
