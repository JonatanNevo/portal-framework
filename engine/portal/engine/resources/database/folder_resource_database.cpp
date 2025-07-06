//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "folder_resource_database.h"

#include <fstream>

#include "portal/core/buffer_stream.h"
#include "../../../../../core/portal/core/files/file_system.h"
#include "../../../../../serialization/portal/archive/impl/json_archive.h"

namespace portal
{

auto logger = Log::get_logger("ResourceDB");


FolderResourceDatabase::FolderResourceDatabase(const std::filesystem::path& path): root_path(path)
{
    if (!FileSystem::is_directory(path) && !FileSystem::create_directory(path))
    {
        LOGGER_ERROR("Failed to initialize resource database directory: {}", path.string());
        return;
    }

    // searches for an archive file in the root path
    ResourceArchive resource_archive;
    const auto archive_path = path / "root.par";
    if (!FileSystem::exists(archive_path))
    {
        LOGGER_INFO("No archive file found in resource database directory, initializing new archive: {}", archive_path.string());
        std::ofstream archive_file(archive_path, std::ios::binary);
        if (!archive_file.is_open())
        {
            LOGGER_ERROR("Failed to create archive file: {}", archive_path.string());
            return;
        }

        JsonArchiver archiver(archive_file);
        resource_archive.archive(archiver);
    }
    else
    {
        auto data = FileSystem::read_file_binary(archive_path);
        if (data.size == 0)
        {
            LOGGER_ERROR("Failed to read archive file: {}", archive_path.string());
            return;
        }
        BufferStreamReader buffer_stream(data);
        JsonDearchiver json_dearchiver{buffer_stream};
        json_dearchiver.load();

        resource_archive = ResourceArchive::dearchive(json_dearchiver);
    }
    LOGGER_DEBUG("Resource database loaded with {} resources", resource_archive.resources.size());
}

} // portal
