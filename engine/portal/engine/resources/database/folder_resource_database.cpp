//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "folder_resource_database.h"

#include <fstream>

#include "portal/core/buffer_stream.h"
#include "portal/core/files/file_system.h"
#include "portal/engine/resources/source/file_source.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{

static auto logger = Log::get_logger("Resources");

void ResourceArchive::archive(ArchiveObject& archive) const
{
    archive.add_property("id", id.id);
    archive.add_property("name", id.string);
    // archive.add_property("resources", resources);
}

ResourceArchive ResourceArchive::dearchive(ArchiveObject& archive)
{
    uint64_t id = 0;
    std::string name;
    std::unordered_map<StringId, std::filesystem::path> resources;
    archive.get_property<uint64_t>("id", id);
    archive.get_property<std::string>("name", name);
    // archive.get_property<std::unordered_map<StringId, std::filesystem::path>>("resources", resources);
    return {{id, name}, std::move(resources)};
}

FolderResourceDatabase::FolderResourceDatabase(const std::filesystem::path& path): root_path(path)
{
    auto abs_path = std::filesystem::absolute(path);
    if (!FileSystem::is_directory(abs_path) && !FileSystem::create_directory(abs_path))
    {
        LOGGER_ERROR("Failed to initialize resource database directory: {}", abs_path.string());
        return;
    }

    // searches for an archive file in the root path
    ResourceArchive resource_archive;
    auto archive_path = abs_path / "root.par";
    if (!FileSystem::exists(archive_path))
    {
        JsonArchive archiver;
        resource_archive.archive(archiver);
        archiver.dump(archive_path);
    }
    else
    {
        JsonArchive json_dearchiver;
        json_dearchiver.read(archive_path);

        resource_archive = ResourceArchive::dearchive(json_dearchiver);
    }
    LOGGER_DEBUG("Resource database loaded with {} resources", resource_archive.resources.size());
}

std::shared_ptr<resources::ResourceSource> FolderResourceDatabase::get_source(const StringId id) const
{
    // TODO: work with "imports" from the resource archive instead of walking on the path.
    return std::make_shared<resources::FileSource>(absolute(root_path) / id.string);
}

} // portal
