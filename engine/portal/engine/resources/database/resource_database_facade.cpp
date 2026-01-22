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

void ResourceDatabaseFacade::register_database(const Project& project, const DatabaseDescription& description)
{
    auto&& database = ResourceDatabaseFactory::create(project, description);
    PORTAL_ASSERT(database, "Failed to create database");
    databases.emplace(database->get_name(), std::move(database));
}

std::expected<SourceMetadata, DatabaseError> ResourceDatabaseFacade::find(const StringId resource_id)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
        return databases.at(prefix)->find(resource_id);

    LOGGER_ERROR("Cannot find database named: '{}'", prefix);
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

std::unique_ptr<resources::ResourceSource> ResourceDatabaseFacade::create_source(StringId resource_id, SourceMetadata meta)
{
    const auto prefix = find_database_prefix(resource_id);
    if (databases.contains(prefix))
    {
        return databases.at(prefix)->create_source(resource_id, meta);
    }

    LOGGER_ERROR("Cannot find database named: '{}'", prefix);
    return nullptr;
}
} // portal
