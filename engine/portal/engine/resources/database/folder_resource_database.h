//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/engine/resources/database/resource_database.h"
#include "portal/engine/strings/string_registry.h"
#include "../../../../../serialization/portal/archive/archive.h"

namespace portal
{

/**
 * A metadata for a single resource.
 * Each resource will have a metadata file associated with it, located in the same folder as the resource.
 * All metadata files are registered to a root archive file that contains the links to the metadata files.
 */
struct ResourceMetadata
{
    StringId id = INVALID_STRING_ID;
    ResourceType type = ResourceType::Unknown;
};

struct ResourceArchive
{
    StringId id = STRING_ID("root");
    std::unordered_map<StringId, std::filesystem::path> resources{};
};


class FolderResourceDatabase final : public resources::ResourceDatabase
{
public:
    FolderResourceDatabase(const std::filesystem::path& path);

    Resource* get_default_resource(StringId id) const override;
    std::weak_ptr<resources::ResourceSource> get_source(StringId id) const override;

private:
    std::filesystem::path root_path;
};

} // portal