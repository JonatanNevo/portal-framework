//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/core/buffer.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/strings/string_id.h"

namespace portal::resources
{

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

struct SourceMetadata
{
    StringId source_id;
    ResourceType resource_type = ResourceType::Unknown;
    SourceFormat format = SourceFormat::Unknown;
    size_t size;
    std::filesystem::path source_path;
};

class ResourceSource
{
public:
    virtual ~ResourceSource() = default;
    [[nodiscard]] virtual SourceMetadata get_meta() const = 0;
    virtual Buffer load() = 0;
    virtual Buffer load(size_t offset, size_t size) = 0;
    virtual std::unique_ptr<std::istream> stream() = 0;
};

} // portal

namespace fmt
{
template <>
struct formatter<portal::resources::SourceFormat>
{
    static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const portal::resources::SourceFormat& format, FormatContext& ctx) const
    {
        const char* name = "Unknown";
        switch (format)
        {
        case portal::resources::SourceFormat::PrecompiledShader:
            name = "PrecompiledShader";
            break;
        case portal::resources::SourceFormat::Image:
            name = "Image";
            break;
        case portal::resources::SourceFormat::Texture:
            name = "Texture";
            break;
        case portal::resources::SourceFormat::Material:
            name = "Material";
            break;
        case portal::resources::SourceFormat::Obj:
            name = "Obj";
            break;
        case portal::resources::SourceFormat::Shader:
            name = "Shader";
            break;
        case portal::resources::SourceFormat::Glft:
            name = "Glft";
            break;
        case portal::resources::SourceFormat::Memory:
            name = "Memory";
            break;
        case portal::resources::SourceFormat::Unknown:
            name = "Unknown";
            break;
        }
        return fmt::format_to(ctx.out(), "{}", name);
    }
};
}
