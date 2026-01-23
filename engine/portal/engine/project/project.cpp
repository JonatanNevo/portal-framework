//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "project.h"

#include <utility>

#include "portal/core/files/file_system.h"
#include "portal/engine/reference.h"

namespace portal
{
static auto logger = Log::get_logger("Project");

Reference<Project> Project::open_project(const ProjectType type, const std::filesystem::path& path)
{
    auto settings = ProjectSettings::create_settings(SettingsArchiveType::Json, path, PORTAL_SETTINGS_FILE_NAME);

    auto properties = settings.get_setting("project", ProjectProperties{});
    auto project = Reference<Project>(new Project(type, properties, path, std::move(settings)));

    return project;
}

std::filesystem::path Project::get_engine_resource_directory()
{
    return std::filesystem::path(PORTAL_ENGINE_LOCATION) / "resources";
}

std::filesystem::path Project::get_engine_config_directory()
{
    return std::filesystem::path(PORTAL_ENGINE_LOCATION) / "config";
}

Project::Project(const ProjectType type, ProjectProperties project_properties, std::filesystem::path working_directory, ProjectSettings&& settings) :
    type(type),
    properties(std::move(project_properties)),
    project_directory(std::move(working_directory)),
    settings(std::move(settings))
{
    LOGGER_INFO("Opened {} project: {}", type, properties.name.string);
    if (type == ProjectType::Runtime)
    {
        properties.resource_directory = "resources";
    }

    if (properties.include_engine_resources)
    {
        resource_database.register_database(*this, {DatabaseType::Folder, get_engine_resource_directory() / "engine"});
    }

    FileSystem::set_working_directory(project_directory);
    for (auto& description : properties.resources)
    {
        resource_database.register_database(*this, description);
    }
}
} // portal
