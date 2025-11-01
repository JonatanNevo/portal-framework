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
    {{".glb", ".gltf"}, {ResourceType::Composite, SourceFormat::Glft}}
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

ResourceType utils::to_resource_type(const std::string_view resource_type)
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
    if (resource_type == "Scene")
        return ResourceType::Scene;
    return ResourceType::Unknown;
}

SourceFormat utils::to_source_format(const std::string_view source_format)
{
    if (source_format == "Memory")
        return SourceFormat::Memory;
    if (source_format == "Image")
        return SourceFormat::Image;
    if (source_format == "Texture")
        return SourceFormat::Texture;
    if (source_format == "Material")
        return SourceFormat::Material;
    if (source_format == "Obj")
        return SourceFormat::Obj;
    if (source_format == "Shader")
        return SourceFormat::Shader;
    if (source_format == "PrecompiledShader")
        return SourceFormat::PrecompiledShader;
    if (source_format == "Glft")
        return SourceFormat::Glft;
    return SourceFormat::Unknown;
}

const char* utils::to_string(const ResourceState resource_state)
{
    switch (resource_state)
    {
    case ResourceState::Unknown:
        return "Unknown";
    case ResourceState::Loaded:
        return "Loaded";
    case ResourceState::Missing:
        return "Missing";
    case ResourceState::Pending:
        return "Pending";
    case ResourceState::Error:
        return "Error";
    }
    return "Unknown";
}

std::string utils::to_string(const ResourceType resource_type)
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
    case ResourceType::Scene:
        return "Scene";
    }
    PORTAL_ASSERT(false, "Unknown resource Type");
    return "Unknown";
}

std::string utils::to_string(const SourceFormat source_format)
{
    switch (source_format)
    {
    case SourceFormat::Unknown:
        return "Unknown";
    case SourceFormat::Memory:
        return "Memory";
    case SourceFormat::Image:
        return "Image";
    case SourceFormat::Texture:
        return "Texture";
    case SourceFormat::Material:
        return "Material";
    case SourceFormat::Obj:
        return "Obj";
    case SourceFormat::Shader:
        return "Shader";
    case SourceFormat::PrecompiledShader:
        return "PrecompiledShader";
    case SourceFormat::Glft:
        return "Glft";
    }
    return "Unknown";
}

}

