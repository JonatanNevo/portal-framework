//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "file_source.h"

#include <unordered_set>
#include <ranges>
#include <vector>

#include "portal/core/files/file_system.h"

namespace portal::resources
{

static std::vector<std::pair<std::unordered_set<std::string_view>, std::pair<ResourceType, SourceFormat>>> g_asset_extensions
{
    {{".png", ".jpg", ".jpeg", ".hdr"}, {ResourceType::Texture, SourceFormat::Image}},
    {{".obj"}, {ResourceType::Mesh, SourceFormat::Obj}},
    {{".mtl"}, {ResourceType::Material, SourceFormat::Material}},
    {{".slang"}, {ResourceType::Shader, SourceFormat::Shader}},
    {{".spv"}, {ResourceType::Shader, SourceFormat::Preprocessed}},
    {{".glb", ".gltf"}, {ResourceType::Composite, SourceFormat::Glft}}
};


static auto logger = Log::get_logger("Resources");

FileSource::FileSource(const std::filesystem::path& path): file_path(path) {}

SourceMetadata FileSource::get_meta() const
{
    if (!std::filesystem::exists(file_path))
    {
        LOGGER_ERROR("Path for resource does not exist: {}", file_path.string());
        return {INVALID_STRING_ID, ResourceType::Unknown, SourceFormat::Unknown};
    }

    const auto file_extension = file_path.extension().string();
    for (auto& [extension_set, resource_type] : g_asset_extensions)
    {
        if (extension_set.contains(file_extension))
        {
            return {
                STRING_ID(file_path.filename().string()),
                resource_type.first,
                resource_type.second
            };
        }
    }

    return {INVALID_STRING_ID, ResourceType::Unknown, SourceFormat::Unknown};
}

Buffer FileSource::load()
{
    if (!std::filesystem::exists(file_path))
    {
        LOGGER_ERROR("Path for resource does not exist: {}", file_path.string());
        return {};
    }

    return FileSystem::read_file_binary(file_path);
}
} // portal
