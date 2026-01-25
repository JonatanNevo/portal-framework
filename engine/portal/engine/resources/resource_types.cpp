//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_types.h"

#include <unordered_set>

namespace portal
{
static std::vector<std::pair<std::unordered_set<std::string_view>, std::pair<ResourceType, SourceFormat>>> g_asset_extensions
{
    {{".png", ".jpg", ".jpeg", ".hdr"}, {ResourceType::Texture, SourceFormat::Image}},
    {{".obj"}, {ResourceType::Mesh, SourceFormat::Obj}},
    {{".mtl"}, {ResourceType::Material, SourceFormat::Material}},
    {{".slang"}, {ResourceType::Shader, SourceFormat::Shader}},
    {{".spv"}, {ResourceType::Shader, SourceFormat::PrecompiledShader}},
    {{".glb", ".gltf"}, {ResourceType::Composite, SourceFormat::Glft}},
    {{".ttf"}, {ResourceType::Font, SourceFormat::FontFile}},
    {{".pscene"}, {ResourceType::Scene, SourceFormat::Scene}}
};


std::optional<std::pair<ResourceType, SourceFormat>> utils::find_extension_type(const std::string_view extension)
{
    for (auto& [extension_set, type_pair] : g_asset_extensions)
    {
        if (extension_set.contains(extension))
            return type_pair;
    }

    LOG_WARN_TAG("Resources", "Failed to find type for extension: {}", extension);
    return std::nullopt;
}
}
