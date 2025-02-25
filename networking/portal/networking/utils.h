//
// Created by Jonatan Nevo on 25/02/2025.
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
