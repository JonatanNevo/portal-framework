//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <string_view>
#include <vector>
#include <steam/steamnetworkingtypes.h>


namespace portal::network
{
bool is_valid_id_address(std::string_view ip);
std::vector<SteamNetworkingIPAddr> resolve_address(std::string_view address);
}
