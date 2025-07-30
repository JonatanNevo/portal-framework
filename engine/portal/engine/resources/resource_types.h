//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <cstdint>
#include <string_view>
#include <portal/core/common.h>
#include <portal/core/debug/assert.h>

#include <spdlog/spdlog.h>

namespace portal
{

enum class ResourceState: uint8_t
{
    Empty   = 0,      // The resource is not loaded, and has no data
    Loaded  = BIT(0), // The resource is loaded and ready to use
    Missing = BIT(1), // The resource was not found in the database
    Invalid = BIT(2), // The resource is not yet valid, e.g. not yet loaded
    Error   = BIT(3), // The resource failed to load.
};

enum class ResourceType: uint16_t
{
    Unknown = 0,
    Material = 1,
    Texture = 2,
    Shader = 3,
    Mesh = 4,
    Composite = std::numeric_limits<uint16_t>::max(),
};

namespace utils
{
    inline ResourceType resource_type_from_string(const std::string_view resource_type)
    {
        if (resource_type == "Texture")
            return ResourceType::Texture;
        if (resource_type == "Material")
            return ResourceType::Material;
        if (resource_type == "Shader")
            return ResourceType::Shader;
        if (resource_type == "Mesh")
            return ResourceType::Mesh;
        if (resource_type == "Composite")
            return ResourceType::Composite;
        return ResourceType::Unknown;
    }

    inline const char* to_string(const ResourceType resource_type)
    {
        switch (resource_type)
        {
        case ResourceType::Unknown:
            return "Unknown";
        case ResourceType::Material:
            return "Material";
        case ResourceType::Texture:
            return "Texture";
        case ResourceType::Shader:
            return "Shader";
        case ResourceType::Mesh:
            return "Mesh";
        case ResourceType::Composite:
            return "Composite";
        }
        PORTAL_ASSERT(false, "Unknown resource Type");
        return "Unknown";
    }
}
}

template <>
struct fmt::formatter<portal::ResourceType>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::ResourceType& type, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", portal::utils::to_string(type));
    }
};

template<>
struct std::hash<portal::ResourceType>
{
    size_t operator()(const portal::ResourceType& type) const noexcept
    {
        return static_cast<uint16_t>(type);
    }
};