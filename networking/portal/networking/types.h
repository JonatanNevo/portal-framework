//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <steam/steamnetworkingtypes.h>

namespace portal::network
{
// See ESteamNetworkingConnectionState
enum class ConnectionState
{
    None,
    Connecting,
    FindingRoute,
    Connected,
    ClosedByPeer,
    ProblemDetectedLocally
};

// See ESteamNetConnectionEnd
enum class ConnectionEnd
{
    // Invalid/sentinel value
    Invalid = 0,

    // 1xxx: Application ended the connection in a "usual" manner.
    //       E.g.: user intentionally disconnected from the server,
    //             gameplay ended normally, etc
    AppGeneric          = 1000,
    AppFinished         = 1001,
    AppConnectionClosed = 1002,
    AppKickedByServer     = 1003,

    // 2xxx: Application ended the connection in some sort of exceptional
    //       or unusual manner that might indicate a bug or configuration
    //       issue.
    //
    AppExceptionGeneric = 2000,

    //
    // System codes.  These will be returned by the system when
    // the connection state is k_ESteamNetworkingConnectionState_ClosedByPeer
    // or k_ESteamNetworkingConnectionState_ProblemDetectedLocally.  It is
    // illegal to pass a code in this range to ISteamNetworkingSockets::CloseConnection
    //

    // 3xxx: Connection failed or ended because of problem with the
    //       local host or their connection to the Internet.
    OfflineMode                  = 3001,
    ManyRelayConnectivity        = 3002,
    HostedServerPrimaryRelay     = 3003,
    NetworkConfig                = 3004,
    Rights                       = 3005,
    LocalP2PICENoPublicAddresses = 3006,

    // 4xxx: Connection failed or ended, and it appears that the
    //       cause does NOT have to do with the local host or their
    //       connection to the Internet.  It could be caused by the
    //       remote host, or it could be somewhere in between.
    RemoteTimeout                 = 4001,
    BadCrypt                      = 4002,
    BadCert                       = 4003,
    BadProtocolVersion            = 4006,
    RemoteP2PICENoPublicAddresses = 4007,

    // 5xxx: Connection failed for some other reason.
    Misc                    = 5001,
    InternalError           = 5002,
    Timeout                 = 5003,
    SteamConnectivity       = 5005,
    NoRelaySessionsToClient = 5006,
    P2PRendezvous           = 5008,
    P2PNATFirewall          = 5009,
    PeerSentNoConnection    = 5010,
};

namespace internal
{
    [[nodiscard]] inline ConnectionState from_steam_networking_state(ESteamNetworkingConnectionState state);

    [[nodiscard]] inline ConnectionEnd from_steam_networking_end(ESteamNetConnectionEnd end)
    {
        return static_cast<ConnectionEnd>(end);
    };

    [[nodiscard]] inline ESteamNetConnectionEnd to_steam_networking_end(ConnectionEnd end)
    {
        return static_cast<ESteamNetConnectionEnd>(end);
    };
}
} // namespace portal::network
