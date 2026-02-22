//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>
#include <llvm/ADT/DenseMap.h>

#include "resource_database.h"
#include "portal/core/files/file_system.h"

namespace portal
{
class Project;
constexpr auto CURRENT_DATABASE_VERSION = 1;

struct DatabaseMetadata
{
    size_t version = CURRENT_DATABASE_VERSION;
    StringId name = STRING_ID("root");
    size_t resource_count = 0;
    std::vector<std::string> ignored_extensions = {};
    std::vector<std::string> ignored_files = {};
    ResourceDirtyFlags dirty = ResourceDirtyBits::DataChange;

    void archive(ArchiveObject& archive) const;
    static DatabaseMetadata dearchive(ArchiveObject& archive);
};

class FolderResourceDatabase final : public ResourceDatabase
{
public:
    constexpr static auto RESOURCE_METADATA_EXTENSION = ".pmeta";
    constexpr static auto DATABASE_METADATA_EXTENSION = ".podb";

public:
    static std::unique_ptr<FolderResourceDatabase> create(const Project& project, const std::filesystem::path& database_path);

    ~FolderResourceDatabase() override;

    std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) override;
    DatabaseError add(StringId resource_id, SourceMetadata meta) override;
    DatabaseError remove(StringId resource_id) override;

    Reference<resources::ResourceSource> create_source(StringId resource_id, SourceMetadata meta) override;

protected:
    FolderResourceDatabase(
        std::filesystem::path root_path,
        std::filesystem::path meta_path,
        DatabaseMetadata metadata
    );

    void populate();
    void populate_from_composite(const SourceMetadata& meta);

    DatabaseError validate();
    [[nodiscard]] DatabaseError validate_metadata(const SourceMetadata& meta) const;

    void mend(DatabaseError error);
    void clean_metadata();

    static void save_meta(const std::filesystem::path& meta_path, DatabaseMetadata& metadata);
    static DatabaseMetadata load_meta(const std::filesystem::path& meta_path);

    [[nodiscard]] StringId get_name() const override;

private:
    std::filesystem::path root_path;
    std::filesystem::path meta_path;
    DatabaseMetadata metadata;

#ifdef PORTAL_DEBUG
    std::unordered_map<StringId, SourceMetadata> resources;
#else
    llvm::DenseMap<StringId, SourceMetadata> resources;
#endif
};
} // portal
