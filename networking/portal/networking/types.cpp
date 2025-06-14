//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "types.h"


namespace portal::network::internal
{
inline ConnectionState from_steam_networking_state(ESteamNetworkingConnectionState state)
{
    switch (state)
    {
    case k_ESteamNetworkingConnectionState_Connecting:
        return ConnectionState::Connecting;
    case k_ESteamNetworkingConnectionState_FindingRoute:
        return ConnectionState::FindingRoute;
    case k_ESteamNetworkingConnectionState_Connected:
        return ConnectionState::Connected;
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
        return ConnectionState::ClosedByPeer;
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        return ConnectionState::ProblemDetectedLocally;
    default:
        return ConnectionState::None;
    }
}
} // namespace portal::network::internal
