//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>
#include <llvm/ADT/DenseMap.h>

#include "resource_database.h"

namespace portal
{
constexpr auto CURRENT_DATABASE_VERSION = 1;

struct DatabaseMetadata
{
    size_t version = CURRENT_DATABASE_VERSION;
    StringId name = STRING_ID("root");
    size_t resource_count = 0;
    ResourceDirtyFlags dirty = ResourceDirtyBits::DataChange;

    void archive(ArchiveObject& archive) const;
    static DatabaseMetadata dearchive(ArchiveObject& archive);
};

class FolderResourceDatabase final : public ResourceDatabase
{
public:
    static std::unique_ptr<FolderResourceDatabase> create(ModuleStack& stack, const std::filesystem::path& base_path);

    ~FolderResourceDatabase() override;

    std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) override;
    DatabaseError add(StringId resource_id, SourceMetadata meta) override;
    DatabaseError remove(StringId resource_id) override;

    std::unique_ptr<resources::ResourceSource> create_source(StringId resource_id, SourceMetadata meta) override;

protected:
    FolderResourceDatabase(
        ModuleStack& stack,
        const StringId& name,
        std::filesystem::path  root_path,
        std::filesystem::path  meta_path,
        DatabaseMetadata metadata
    );

    void populate();

    DatabaseError validate();
    [[nodiscard]] DatabaseError validate_metadata(const SourceMetadata& meta) const;

    void mend(DatabaseError error);
    void clean_metadata();

    static void save_meta(const std::filesystem::path& meta_path, DatabaseMetadata& metadata);
    static DatabaseMetadata load_meta(const std::filesystem::path& meta_path);

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
