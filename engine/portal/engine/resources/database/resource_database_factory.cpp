//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_database_factory.h"

#include "folder_resource_database.h"
#include "resource_database.h"
#include "portal/application/modules/module_stack.h"

namespace portal
{
static auto logger = Log::get_logger("Resources");

void DatabaseDescription::archive(ArchiveObject& archive) const
{
    archive.add_property("type", type);
    if (path.has_value())
        archive.add_property("path", path.value());
}

DatabaseDescription DatabaseDescription::dearchive(ArchiveObject& archive)
{
    std::string name;
    DatabaseType type{};
    archive.get_property("type", type);
    DatabaseDescription description{.type = type};

    std::filesystem::path path;
    if (archive.get_property("path", path))
        description.path = path;

    return description;
}

std::unique_ptr<ResourceDatabase> ResourceDatabaseFactory::create(const DatabaseDescription& description)
{
    switch (description.type)
    {
    case DatabaseType::Unknown:
        LOGGER_ERROR("Unknown database type");
        return nullptr;

    case DatabaseType::Folder:
        PORTAL_ASSERT(description.path.has_value(), "Invalid database description for Folder database");
        return FolderResourceDatabase::create(description.path.value());
    }

    return nullptr;
}
} // portal
