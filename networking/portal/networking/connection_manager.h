//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <map>
#include <mutex>
#include <steam/steamnetworkingtypes.h>

class ISteamNetworkingSockets;

namespace portal::network
{
class Host;
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

    void add_host(HSteamListenSocket host, Host* host_object);
    void remove_host(HSteamListenSocket host);
    Host* get_host(HSteamListenSocket host);

protected:
    ConnectionManager();
    ~ConnectionManager();

private:
    static ConnectionManager* instance;
    static std::mutex instance_mutex;
    ISteamNetworkingSockets* sockets;
    std::map<HSteamNetConnection, Connection*> connections;
    std::map<HSteamListenSocket, Host*> hosts;
};

} // namespace portal::network
