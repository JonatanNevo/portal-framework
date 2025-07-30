//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/strings/string_id.h"

namespace portal::resources
{

enum class SourceFormat: uint8_t
{
    Unknown,
    Preprocessed, // Preprocessed formats, no need to process before loading.
    Image,        // Image formats, e.g. PNG, JPEG
    Texture,      // Ktx or other texture formats
    Material,     // Material files, e.g. MTL
    Obj,          // Wavefront .obj files
    Shader,       // Shader files, e.g. slang
    Glft          // GLTF files
};

struct SourceMetadata
{
    StringId source_id;
    ResourceType resource_type;
    SourceFormat format;
};

class ResourceSource
{
public:
    virtual ~ResourceSource() = default;
    [[nodiscard]] virtual SourceMetadata get_meta() const = 0;
    virtual Buffer load() = 0;
};

} // portal

namespace fmt
{
template<>
struct formatter<portal::resources::SourceFormat>
{
    static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const portal::resources::SourceFormat& format, FormatContext& ctx) const
    {
        const char* name = "Unknown";
        switch (format)
        {
        case portal::resources::SourceFormat::Preprocessed:
            name = "Preprocessed";
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
        default:
            break;
        }
        return std::format_to(ctx.out(), "{}", name);
    }
};
}
