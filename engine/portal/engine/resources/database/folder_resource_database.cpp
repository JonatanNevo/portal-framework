//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "folder_resource_database.h"

#include <unordered_set>

#include "portal/core/files/file_system.h"
#include "../source/file_source.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{

constexpr auto RESOURCE_METADATA_EXTENSION = ".pmeta";
constexpr auto DATABASE_METADATA_EXTENSION = ".podb";

const auto ROOT_DATABASE_METADATA_FILENAME = std::format("root{}", DATABASE_METADATA_EXTENSION);

static auto logger = Log::get_logger("Resources");

void DatabaseMetadata::archive(ArchiveObject& archive) const
{
    archive.add_property("version", version);
    archive.add_property("name", name.string);
    archive.add_property("resource_count", resource_count);
    archive.add_property("dirty", dirty.get());
}

DatabaseMetadata DatabaseMetadata::dearchive(ArchiveObject& archive)
{
    std::string name;
    DatabaseMetadata metadata;
    ResourceDirtyFlags::MaskType dirty;

    archive.get_property("version", metadata.version);
    archive.get_property("name", name);
    metadata.name = STRING_ID(name);
    archive.get_property("resource_count", metadata.resource_count);
    archive.get_property("dirty", dirty);
    metadata.dirty = ResourceDirtyFlags(dirty);

    return metadata;
}

FolderResourceDatabase::FolderResourceDatabase(const std::filesystem::path& path) : root_path(std::filesystem::absolute(path)), meta_path(root_path / ROOT_DATABASE_METADATA_FILENAME)
{
    if (!FileSystem::is_directory(root_path) && !FileSystem::create_directory(root_path))
    {
        LOGGER_ERROR("Failed to initialize resource database directory: {}", root_path.string());
        throw std::runtime_error("Failed to initialize resource database directory");
    }

    // Find database meta, if not, create one
    if (!FileSystem::exists(meta_path))
    {
        save_meta();
        LOGGER_INFO("Creating new folder database {}, version: {}", metadata.name, metadata.version);
    }
    else
    {
        metadata = load_meta();
        LOGGER_INFO("Loaded folder database {}, version: {}", metadata.name, metadata.version);
    }

    populate();
    const auto validate_result = validate();
    mend(validate_result);
}

FolderResourceDatabase::~FolderResourceDatabase()
{
    clean_metadata();

    if (validate() != DatabaseErrorBit::Success)
        LOGGER_ERROR("Folder database destructed in invalid state");
}

std::expected<SourceMetadata, DatabaseError> FolderResourceDatabase::find(const StringId resource_id)
{
    return find(resource_id.id);
}

std::expected<SourceMetadata, DatabaseError> FolderResourceDatabase::find(const ResourceHandle handle)
{
    if (resources.contains(handle))
        return resources.at(handle);

    return std::unexpected{DatabaseErrorBit::MissingResource};
}

DatabaseError FolderResourceDatabase::add(const SourceMetadata meta)
{
    if (resources.contains(meta.handle))
    {
        LOGGER_ERROR("Attempted to add resource with handle {} that already exists", meta.resource_id);
        return DatabaseErrorBit::Conflict;
    }

    if (validate_metadata(meta) != DatabaseErrorBit::Success)
    {
        LOGGER_ERROR("Attempted to add metadata to a resource that does not exist");
        return DatabaseErrorBit::MissingResource;
    }

    const auto source_path = std::filesystem::path(meta.source.string);
    const auto metadata_path = root_path / fmt::format("{}{}", source_path.filename().string(), RESOURCE_METADATA_EXTENSION);

    JsonArchive archiver;
    meta.archive(archiver);
    archiver.dump(metadata_path);

    // TODO: thread safety?
    resources[meta.handle] = meta;

    return DatabaseErrorBit::Success;
}

DatabaseError FolderResourceDatabase::remove(const StringId resource_id)
{
    return remove(resource_id.id);
}

DatabaseError FolderResourceDatabase::remove(const ResourceHandle handle)
{
    if (!resources.contains(handle))
    {
        LOGGER_ERROR("Attempted to remove resource with handle {} that does not exist", handle);
        return DatabaseErrorBit::MissingResource;
    }

    auto meta = resources.at(handle);

    FileSystem::remove(root_path / fmt::format("{}{}", meta.source.string, RESOURCE_METADATA_EXTENSION));
    // TODO: remove resource file as well?

    resources.erase(handle);

    LOGGER_DEBUG("Removed resource with handle: {}", handle);
    return DatabaseErrorBit::Success;
}

std::unique_ptr<resources::ResourceSource> FolderResourceDatabase::create_source(SourceMetadata meta)
{
    // TODO: if source starts with 'http://' use network source
    return std::make_unique<resources::FileSource>(root_path / meta.source.string);
}

void FolderResourceDatabase::populate()
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(root_path))
    {
        // TODO: support links?
        if (entry.is_regular_file())
        {
            // TODO: handle nested databases
            if (entry.path().extension() == RESOURCE_METADATA_EXTENSION)
            {
                JsonArchive archiver;
                archiver.read(entry.path());

                // TODO: Add serialization checks
                auto resource_metadata = SourceMetadata::dearchive(archiver);
                resources[resource_metadata.handle] = resource_metadata;

                // TODO: update to new version if necessary
                resource_metadata.archive(archiver);
                archiver.dump(entry.path());
            }
        }
    }
}

DatabaseError FolderResourceDatabase::validate()
{
    DatabaseError error;

    if (metadata.dirty != ResourceDirtyBits::Clean)
    {
        LOGGER_ERROR("Metadata is dirty, might not reflect the folder state");
    }

    if (resources.size() != metadata.resource_count)
    {
        LOGGER_ERROR("Invalid amount of resource in database, expected: {}, found: {}", metadata.resource_count, resources.size());
        error |= DatabaseErrorBit::NotFound;
    }

    std::unordered_map<StringId, bool> corresponding_meta;
    corresponding_meta.reserve(resources.size());
    for (auto& [handle, meta] : resources)
        corresponding_meta[meta.source] = false;

    std::unordered_set<StringId> missing_metadata;

    for (auto& entry : std::filesystem::recursive_directory_iterator(root_path))
    {
        // TODO: support links?
        if (entry.is_regular_file())
        {
            if (entry.path().extension() != RESOURCE_METADATA_EXTENSION && entry.path().extension() != DATABASE_METADATA_EXTENSION)
            {
                auto file_as_string_id = STRING_ID(entry.path().filename().string());
                if (corresponding_meta.contains(file_as_string_id))
                    corresponding_meta[file_as_string_id] = true;
                else
                    missing_metadata.insert(file_as_string_id);
            }
        }
    }

    // TODO: Check if meta has references, if no, remove
    if (!std::ranges::all_of(corresponding_meta | std::views::values, [](const bool value) { return value; }))
    {
        LOGGER_WARN("Found some stale metadate in database");
        error |= DatabaseErrorBit::StaleMetadata;
    }

    // TODO: attempt to generate meta
    if (!missing_metadata.empty())
    {
        LOGGER_WARN("There are {} resources without metadata in database", missing_metadata.size());
        error |= DatabaseErrorBit::MissingMetadata;
    }

    return error;
}

DatabaseError FolderResourceDatabase::validate_metadata(const SourceMetadata& meta) const
{
    const auto resource_path = root_path / std::filesystem::path(meta.source.string);

    if (!FileSystem::exists(resource_path))
        return DatabaseErrorBit::MissingResource;

    return DatabaseErrorBit::Success;
}

void FolderResourceDatabase::mend(const DatabaseError error)
{
    if (error & DatabaseErrorBit::MissingResource)
    {
        metadata.resource_count = resources.size();
        // TODO: attempt to add new resources
    }

    metadata.dirty |= ResourceDirtyBits::DataChange;
    save_meta();
}

void FolderResourceDatabase::clean_metadata()
{
    metadata.resource_count = resources.size();
    metadata.dirty = ResourceDirtyBits::Clean;
    save_meta();
}

void FolderResourceDatabase::save_meta() const
{
    JsonArchive archiver;
    metadata.archive(archiver);
    archiver.dump(meta_path);
}

DatabaseMetadata FolderResourceDatabase::load_meta() const
{
    JsonArchive json_dearchiver;
    json_dearchiver.read(meta_path);

    return DatabaseMetadata::dearchive(json_dearchiver);
}

} // portal
