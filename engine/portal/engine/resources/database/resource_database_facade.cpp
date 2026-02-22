//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_database_facade.h"

#include <ranges>

#include "portal/engine/resources/source/resource_source.h"

namespace portal
{
static auto logger = Log::get_logger("Resources");

StringId find_database_prefix(const StringId& resource_id)
{
    auto split_view = resource_id.string | std::views::split('/');
    PORTAL_ASSERT(std::ranges::distance(split_view) > 1, "Invalid resource id");

    auto first_part = std::string_view(*split_view.begin());

    return STRING_ID(first_part);
}

ResourceDatabaseFacade::ResourceDatabaseFacade(): structure(STRING_ID("root")) {}

void ResourceDatabaseFacade::register_database(const Project& project, const DatabaseDescription& description)
{
    auto&& database = ResourceDatabaseFactory::create(project, description);
    PORTAL_ASSERT(database, "Failed to create database");
    auto& db_structure = database->get_structure();

    structure.children[db_structure.name] = make_reference<resources::FacadeDatabaseEntry>(db_structure.name, &structure);
    auto& facade_entry = structure.children[db_structure.name];
    facade_entry->children = db_structure.children;

    databases.emplace(database->get_name(), std::move(database));
}

std::expected<SourceMetadata, DatabaseError> ResourceDatabaseFacade::find(const StringId resource_id)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
        return databases.at(prefix)->find(resource_id);

    return std::unexpected{DatabaseErrorBit::DatabaseMissing};
}

DatabaseError ResourceDatabaseFacade::add(StringId resource_id, SourceMetadata meta)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
        return databases.at(prefix)->add(resource_id, meta);

    LOGGER_ERROR("Cannot find database named: '{}'", prefix);
    return DatabaseErrorBit::DatabaseMissing;
}

DatabaseError ResourceDatabaseFacade::remove(const StringId resource_id)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
        return databases.at(prefix)->remove(resource_id);

    LOGGER_ERROR("Cannot find database named: '{}'", prefix);
    return DatabaseErrorBit::DatabaseMissing;
}

Reference<resources::ResourceSource> ResourceDatabaseFacade::create_source(StringId resource_id, SourceMetadata meta)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
    {
        return databases.at(prefix)->create_source(resource_id, meta);
    }

    LOGGER_ERROR("Cannot find database named: '{}'", prefix);
    return nullptr;
}
const std::filesystem::path& ResourceDatabaseFacade::get_root_path() const
{
    static const std::filesystem::path empty_path;
    return empty_path;
}

ResourceDatabase& ResourceDatabaseFacade::get_database(StringId name) const
{
    auto it = databases.find(name);
    PORTAL_ASSERT(it != databases.end(), "Database not found: {}", name);
    return *it->second;
}
} // portal
