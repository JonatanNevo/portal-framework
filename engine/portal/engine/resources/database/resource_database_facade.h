//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <stack>

#include "resource_database.h"
#include "resource_database_factory.h"

namespace portal
{
namespace resources
{
    struct FacadeDatabaseEntry final : DatabaseEntry
    {
        using DatabaseEntry::DatabaseEntry;

        std::filesystem::path get_path() const override { return {}; }
    };
}

class ResourceDatabaseFacade final : public ResourceDatabase
{
public:
    ResourceDatabaseFacade();

    void register_database(const Project& project, const DatabaseDescription& description);

    std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) override;
    Reference<resources::ResourceSource> create_source(StringId resource_id, SourceMetadata meta) override;

    DatabaseError add(StringId resource_id, SourceMetadata meta) override;
    DatabaseError remove(StringId resource_id) override;

    [[nodiscard]] StringId get_name() const override { return STRING_ID("Resource Database Facade"); }
    [[nodiscard]] resources::DatabaseEntry& get_structure() const override { return const_cast<resources::FacadeDatabaseEntry&>(structure); }
    [[nodiscard]] const std::filesystem::path& get_root_path() const override;

    [[nodiscard]] ResourceDatabase& get_database(StringId name) const;

private:
    std::unordered_map<StringId, std::unique_ptr<ResourceDatabase>> databases;
    resources::FacadeDatabaseEntry structure;
};

} // portal
