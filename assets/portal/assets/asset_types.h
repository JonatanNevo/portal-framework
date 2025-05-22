//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once

#include <portal/core/common.h>
#include <portal/core/assert.h>

namespace portal
{

enum class AssetState: uint8_t
{
    Empty = 0,
    Loaded = BIT(0),
    Missing = BIT(1),
    Invalid = BIT(2),
};

enum class AssetType: uint16_t
{
    Undefined = 0,
    Texture
};

namespace utils
{
    inline AssetType from_string(const std::string_view asset_type)
    {
        if (asset_type == "Texture")
            return AssetType::Texture;
        return AssetType::Undefined;
    }

    inline const char* to_string(const AssetType asset_type)
    {
        switch (asset_type)
        {
        case AssetType::Texture:
            return "Texture";
        case AssetType::Undefined:
            return "Undefined";
        }

        PORTAL_CORE_ASSERT(false, "Undefined Asset Type");
        return "Undefined";
    }
}
}
