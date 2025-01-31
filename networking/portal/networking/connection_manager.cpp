//
// Created by Jonatan Nevo on 31/01/2025.
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
        if(instance == nullptr)
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

    void ConnectionManager::add_host(const HSteamListenSocket host, Host* host_object)
    {
        if (hosts.contains(host))
            throw std::runtime_error("Host already exists");
        hosts[host] = host_object;
    }

    void ConnectionManager::remove_host(const HSteamListenSocket host)
    {
        if (!hosts.contains(host))
            return;
        hosts.erase(host);
    }

    Host* ConnectionManager::get_host(const HSteamListenSocket host)
    {
        if (!hosts.contains(host))
            return nullptr;
        return hosts[host];
    }

    void ConnectionManager::remove_connection(const HSteamNetConnection connection) {
        if(!connections.contains(connection))
            return;
        connections.erase(connection);
    }


} // namespace portal::network
