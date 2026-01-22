//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>
#include <optional>

#include "portal/core/strings/string_id.h"

namespace portal
{
class Project;

enum class DatabaseType
{
    Unknown,
    Folder
};

class ModuleStack;
class ResourceDatabase;
class ArchiveObject;

struct DatabaseDescription
{
    DatabaseType type = DatabaseType::Unknown;
    std::optional<std::filesystem::path> path;

    void archive(ArchiveObject& archive) const;
    static DatabaseDescription dearchive(ArchiveObject& archive);
};



class ResourceDatabaseFactory
{
public:
    static std::unique_ptr<ResourceDatabase> create(const Project& project, const DatabaseDescription& description);
};
} // portal