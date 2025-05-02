//
// Created by thejo on 4/11/2025.
//

#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "asset_types.h"

namespace portal::assets
{
using namespace std::string_view_literals;

inline static std::unordered_map<std::unordered_set<std::string_view>, AssetType> g_asset_extensions
{
    {{".png", ".jpg", ".jpeg", ".hdr"}, AssetType::Texture}
};

}
