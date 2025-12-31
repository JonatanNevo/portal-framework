//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "folder_resource_database.h"

#include <unordered_set>
#include <utility>

#include "portal/core/files/file_system.h"
#include "portal/engine/resources/loader/loader_factory.h"
#include "portal/engine/resources/source/file_source.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{
using namespace std::literals;

constexpr auto RESOURCE_METADATA_EXTENSION = ".pmeta";
constexpr auto DATABASE_METADATA_EXTENSION = ".podb";
constexpr std::array IGNORED_EXTENSIONS = {".bin"sv};

const auto ROOT_DATABASE_METADATA_FILENAME = fmt::format("root{}", DATABASE_METADATA_EXTENSION);

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

std::filesystem::path validate_and_create_path(const std::filesystem::path& base_path)
{
    std::filesystem::path output;

    if (base_path.is_absolute())
        output = base_path;
    else
        output = std::filesystem::absolute("resources") / base_path;

    if (!FileSystem::is_directory(output) && !FileSystem::create_directory(output))
    {
        LOGGER_ERROR("Failed to initialize resource database directory: {}", output.generic_string());
        throw std::runtime_error("Failed to initialize resource database directory");
    }

    return output;
}

std::filesystem::path validate_and_create_meta_path(const std::filesystem::path& root_path)
{
    std::vector<std::filesystem::path> meta_files;

    for (const auto& entry : std::filesystem::directory_iterator(root_path))
    {
        if (entry.is_regular_file() && entry.path().extension() == DATABASE_METADATA_EXTENSION)
        {
            meta_files.push_back(entry.path());
        }
    }

    if (meta_files.empty())
    {
        LOGGER_ERROR("Failed to find database metadata file in {}", root_path.generic_string());
        throw std::runtime_error("Failed to find database metadata file in root path");
    }

    if (meta_files.size() > 1)
    {
        LOGGER_ERROR("Found {} database metadata files in {}, expected exactly one", meta_files.size(), root_path.generic_string());
        throw std::runtime_error("Multiple database metadata files found in root path");
    }

    return meta_files[0];
}

std::unique_ptr<FolderResourceDatabase> FolderResourceDatabase::create(ModuleStack& stack, const std::filesystem::path& base_path)
{
    const auto root_path = validate_and_create_path(base_path);
    const auto meta_path = validate_and_create_meta_path(root_path);
    const auto metadata = load_meta(meta_path);
    return std::unique_ptr<FolderResourceDatabase>(new FolderResourceDatabase(stack, metadata.name, root_path, meta_path, metadata));
}

FolderResourceDatabase::FolderResourceDatabase(
    ModuleStack& stack,
    const StringId& name,
    std::filesystem::path root_path,
    std::filesystem::path meta_path,
    DatabaseMetadata metadata
) : ResourceDatabase(stack, name),
    root_path(std::move(root_path)),
    meta_path(std::move(meta_path)),
    metadata(metadata)
{
    LOGGER_INFO("Loaded folder database {}, version: {}", metadata.name, metadata.version);

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
    if (resources.contains(resource_id))
        return resources.at(resource_id);

    return std::unexpected{DatabaseErrorBit::MissingResource};
}

DatabaseError FolderResourceDatabase::add(StringId resource_id, SourceMetadata meta)
{
    if (resources.contains(resource_id))
    {
        LOGGER_ERROR("Attempted to add resource with handle {} that already exists", resource_id);
        return DatabaseErrorBit::Conflict;
    }

    if (validate_metadata(meta) != DatabaseErrorBit::Success)
    {
        LOGGER_ERROR("Attempted to add metadata to a resource that does not exist");
        return DatabaseErrorBit::MissingResource;
    }


    const auto source_path = std::filesystem::path(meta.source.string);
    const auto metadata_path = root_path / fmt::format("{}{}", source_path.generic_string(), RESOURCE_METADATA_EXTENSION);
    meta.full_source_path = STRING_ID((root_path / source_path).generic_string());

    const resources::FileSource source(root_path / source_path);
    resources::LoaderFactory::enrich_metadata(meta, source);

    JsonArchive archiver;
    meta.archive(archiver);
    archiver.dump(metadata_path);

    // TODO(#45): thread safety?
    resources[resource_id] = meta;

    return DatabaseErrorBit::Success;
}

DatabaseError FolderResourceDatabase::remove(const StringId resource_id)
{
    if (!resources.contains(resource_id))
    {
        LOGGER_ERROR("Attempted to remove resource with handle {} that does not exist", resource_id);
        return DatabaseErrorBit::MissingResource;
    }

    auto meta = resources.at(resource_id);

    FileSystem::remove(root_path / fmt::format("{}{}", meta.source.string, RESOURCE_METADATA_EXTENSION));
    // TODO: remove resource file as well?

    resources.erase(resource_id);

    LOGGER_DEBUG("Removed resource with handle: {}", resource_id);
    return DatabaseErrorBit::Success;
}


std::unique_ptr<resources::ResourceSource> FolderResourceDatabase::create_source(StringId, const SourceMetadata meta)
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
                resource_metadata.full_source_path = STRING_ID((root_path / entry.path()).generic_string());
                resources[resource_metadata.resource_id] = resource_metadata;

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
    for (auto& [_, meta] : resources)
        corresponding_meta[meta.source] = false;

    std::unordered_set<StringId> missing_metadata;
    std::unordered_set<StringId> corrupt_meta;

    for (auto& entry : std::filesystem::recursive_directory_iterator(root_path))
    {
        // TODO: support links?
        if (entry.is_regular_file())
        {
            if (entry.path().extension() != RESOURCE_METADATA_EXTENSION && entry.path().extension() != DATABASE_METADATA_EXTENSION &&
                std::ranges::none_of(IGNORED_EXTENSIONS, [&entry](const auto& ext) { return entry.path().extension() == ext; }))
            {
                auto relative_path = std::filesystem::relative(entry.path(), root_path);
                auto file_as_string_id = STRING_ID(relative_path.generic_string());
                if (corresponding_meta.contains(file_as_string_id))
                    corresponding_meta[file_as_string_id] = true;
                else
                    missing_metadata.insert(file_as_string_id);
            }
            else if (entry.path().extension() == RESOURCE_METADATA_EXTENSION)
            {
                auto relative_path = std::filesystem::relative(entry.path(), root_path);
                auto file_as_string_id = STRING_ID(relative_path.generic_string());

                JsonArchive archiver;
                archiver.read(entry.path());
                SourceMetadata meta = SourceMetadata::dearchive(archiver);
                if (validate_metadata(meta) != DatabaseErrorBit::Success)
                {
                    LOGGER_WARN("Corrupt metadata: {}", relative_path.generic_string());
                    corrupt_meta.insert(file_as_string_id);
                }
            }
        }
    }

    // TODO: Check if meta has references, if no, remove
    if (!std::ranges::all_of(corresponding_meta | std::views::values, [](const bool value) { return value; }))
    {
        LOGGER_WARN("Found some stale metadate in database");
        error |= DatabaseErrorBit::StaleMetadata;
    }

    if (!corrupt_meta.empty())
    {
        LOGGER_WARN("Found some corrupt metadate in database");
        error |= DatabaseErrorBit::CorruptMetadata;
    }

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

    const auto expected_resource_id = STRING_ID(
        fmt::format("{}/{}", get_name().string, std::filesystem::path(meta.source.string).replace_extension("").generic_string())
    );

    if (meta.resource_id != expected_resource_id)
        return DatabaseErrorBit::CorruptMetadata;

    return DatabaseErrorBit::Success;
}

void FolderResourceDatabase::mend(const DatabaseError error)
{
    if (error & DatabaseErrorBit::MissingResource)
    {
        metadata.resource_count = resources.size();
        metadata.dirty |= ResourceDirtyBits::DataChange;
    }

    if (error & DatabaseErrorBit::MissingMetadata)
    {
        std::unordered_map<StringId, bool> corresponding_meta;
        corresponding_meta.reserve(resources.size());
        for (auto& [_, meta] : resources)
            corresponding_meta[meta.source] = false;

        for (auto& entry : std::filesystem::recursive_directory_iterator(root_path))
        {
            // TODO: support links?
            if (entry.is_regular_file())
            {
                if (entry.path().extension() != RESOURCE_METADATA_EXTENSION && entry.path().extension() != DATABASE_METADATA_EXTENSION &&
                    std::ranges::none_of(IGNORED_EXTENSIONS, [&entry](const auto& ext) { return entry.path().extension() == ext; }))
                {
                    auto relative_path = std::filesystem::relative(entry.path(), root_path);
                    auto file_as_string_id = STRING_ID(relative_path.generic_string());
                    if (corresponding_meta.contains(file_as_string_id))
                        continue;

                    LOGGER_DEBUG("Creating metadata for resource: {}", file_as_string_id);
                    auto extension = relative_path.extension();
                    auto value = utils::find_extension_type(extension.generic_string());
                    if (!value.has_value())
                        continue;
                    auto [resource_type, source_format] = value.value();

                    // TODO: calculate dependencies?
                    auto resource_id = STRING_ID(
                        fmt::format("{}/{}", get_name().string, relative_path.replace_extension("").generic_string())
                    );
                    SourceMetadata meta{
                        .resource_id = resource_id,
                        .type = resource_type,
                        .source = file_as_string_id,
                        .format = source_format,
                    };

                    add(meta.resource_id, meta);
                }
            }
        }

        metadata.dirty |= ResourceDirtyBits::DataChange;
    }

    if (error & DatabaseErrorBit::CorruptMetadata)
    {
        for (auto& entry : std::filesystem::recursive_directory_iterator(root_path))
        {
            // TODO: support links?
            if (entry.is_regular_file())
            {
                if (entry.path().extension() == RESOURCE_METADATA_EXTENSION)
                {
                    auto relative_path = std::filesystem::relative(entry.path(), root_path);

                    JsonArchive archiver;
                    archiver.read(entry.path());
                    SourceMetadata meta = SourceMetadata::dearchive(archiver);
                    if (validate_metadata(meta) & DatabaseErrorBit::CorruptMetadata)
                    {
                        LOGGER_DEBUG("Mending corrupt metadata: {}", relative_path.generic_string());
                        remove(meta.resource_id);

                        meta.resource_id = STRING_ID(
                            fmt::format("{}/{}", get_name().string, relative_path.replace_extension("").generic_string())
                        );

                        add(meta.resource_id, meta);
                    }
                }
            }
        }

        metadata.dirty |= ResourceDirtyBits::DataChange;
    }

    // TODO: delete stale metadata

    save_meta(meta_path, metadata);
}

void FolderResourceDatabase::clean_metadata()
{
    metadata.resource_count = resources.size();
    metadata.dirty = ResourceDirtyBits::Clean;
    save_meta(meta_path, metadata);
}

void FolderResourceDatabase::save_meta(const std::filesystem::path& meta_path, DatabaseMetadata& metadata)
{
    JsonArchive archiver;
    metadata.archive(archiver);
    archiver.dump(meta_path);
}

DatabaseMetadata FolderResourceDatabase::load_meta(const std::filesystem::path& meta_path)
{
    JsonArchive json_dearchiver;
    json_dearchiver.read(meta_path);

    return DatabaseMetadata::dearchive(json_dearchiver);
}
} // portal
