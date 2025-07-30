//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "utils.h"

#include "portal/core/string_utils.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "portal/core/debug/assert.h"


namespace portal::network
{

bool is_valid_id_address(std::string_view ip)
{
    const std::string ip_address(ip);

    SteamNetworkingIPAddr steam_ip;
    return steam_ip.ParseString(ip_address.c_str());
}

std::vector<SteamNetworkingIPAddr> resolve_address(std::string_view address)
{
    std::vector<SteamNetworkingIPAddr> result;
    result.reserve(2);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return result;
    }
#endif

    bool has_port = address.find(':') != std::string::npos;
    std::string domain, port;
    if (has_port)
    {
        auto domain_and_port = split(address, ':');
        if (domain_and_port.size() != 2)
            return {};
        domain = domain_and_port[0];
        port = domain_and_port[1];
        address = domain;
    }

    struct addrinfo hints = {}, *addresses;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(address.data(), nullptr, &hints, &addresses) != 0)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        return result;
    }

    for (const struct addrinfo* addr = addresses; addr != nullptr; addr = addr->ai_next)
    {
        char ip[INET6_ADDRSTRLEN];
        void* addr_ptr;

        if (addr->ai_family == AF_INET)
        {
            const auto ipv4 = reinterpret_cast<struct sockaddr_in*>(addr->ai_addr);
            addr_ptr = &(ipv4->sin_addr);
        }
        else
        {
            const auto ipv6 = reinterpret_cast<struct sockaddr_in6*>(addr->ai_addr);
            addr_ptr = &(ipv6->sin6_addr);
        }

        inet_ntop(addr->ai_family, addr_ptr, ip, sizeof(ip));
        auto& ip_address = result.emplace_back();
        if (has_port)
        {
            [[maybe_unused]] const bool success = ip_address.ParseString(std::format("{}:{}", ip, port).c_str());
            PORTAL_ASSERT(success, "Failed to parse IP address {}:{}", ip, port);
        }
        else
        {
            [[maybe_unused]] const bool success = ip_address.ParseString(ip);
            PORTAL_ASSERT(success, "Failed to parse IP address {}", ip);
        }
    }

    freeaddrinfo(addresses);
#ifdef _WIN32
    WSACleanup();
#endif

    return result;
}
}
