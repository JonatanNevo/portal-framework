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
    Unknown = 0,
    Loaded  = 1,
    Missing = 2,
    Pending = 3,
    Error   = 4,
    Null
};

enum class ResourceType: uint16_t
{
    Unknown   = 0,
    Material  = 1,
    Texture   = 2,
    Shader    = 3,
    Mesh      = 4,
    Scene     = 6,
    Composite = 7,
};

enum class SourceFormat: uint8_t
{
    Unknown,
    Memory,            // Source exists in memory
    Image,             // Image formats, e.g. PNG, JPEG
    Texture,           // Ktx or other texture formats
    Material,          // Material files, e.g. MTL
    Obj,               // Wavefront .obj files
    Shader,            // Shader files, e.g. slang
    PrecompiledShader, // Precompiled shader files, e.g. spv
    Glft               // GLTF files
};

namespace utils
{
    std::optional<std::pair<ResourceType, SourceFormat>> find_extension_type(std::string_view extension);

    ResourceType to_resource_type(std::string_view resource_type);
    SourceFormat to_source_format(std::string_view source_format);

    const char* to_string(ResourceState resource_state);
    std::string to_string(ResourceType resource_type);
    std::string to_string(SourceFormat source_format);
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

template <>
struct fmt::formatter<portal::SourceFormat>
{
    static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const portal::SourceFormat& format, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", portal::utils::to_string(format));
    }
};

template <>
struct fmt::formatter<portal::ResourceState>
{
    static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const portal::ResourceState& format, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", portal::utils::to_string(format));
    }
};

template <>
struct std::hash<portal::ResourceType>
{
    size_t operator()(const portal::ResourceType& type) const noexcept
    {
        return static_cast<uint16_t>(type);
    }
};
