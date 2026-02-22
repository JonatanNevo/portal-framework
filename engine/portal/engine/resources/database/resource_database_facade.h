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

class ResourceDatabaseFacade final : public ResourceDatabase
{
public:
    ResourceDatabaseFacade();

    void register_database(const Project& project, const DatabaseDescription& description);

    std::expected<SourceMetadata, DatabaseError> find(StringId resource_id) override;
    Reference<resources::ResourceSource> create_source(StringId resource_id,SourceMetadata meta) override;

    DatabaseError add(StringId resource_id, SourceMetadata meta) override;
    DatabaseError remove(StringId resource_id) override;

    [[nodiscard]] StringId get_name() const override { return STRING_ID("Resource Database Facade"); }
private:
    std::unordered_map<StringId, std::unique_ptr<ResourceDatabase>> databases;
};

} // portal