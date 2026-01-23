//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <filesystem>

#include "portal/application/settings.h"
#include "portal/engine/reference.h"
#include "portal/engine/resources/database/resource_database_facade.h"

namespace portal
{
class ResourceDatabaseFacade;

enum class ProjectType
{
    Editor,
    Runtime
};

struct ProjectProperties
{
    StringId name;

    std::filesystem::path resource_directory = "";
    std::filesystem::path config_directory = "config";

    StringId starting_scene;
    std::vector<DatabaseDescription> resources;

    bool include_engine_resources = true;
};

/**
 * The project class holds information such as working directory, project type, and settings.
 */
class Project
{
public:
    static Reference<Project> open_project(ProjectType type, const std::filesystem::path& path);

    [[nodiscard]] ProjectType get_type() const { return type; }
    [[nodiscard]] ProjectSettings& get_settings() const { return const_cast<ProjectSettings&>(settings); }
    [[nodiscard]] const StringId& get_name() const { return properties.name; }
    [[nodiscard]] const StringId& get_starting_scene() const { return properties.starting_scene; }

    [[nodiscard]] const std::filesystem::path& get_project_directory() const { return project_directory; }
    // TODO: Change to bundle resources path once macos bundle configuration is fixed
    [[nodiscard]] std::filesystem::path get_resource_directory() const { return project_directory / properties.resource_directory; }
    [[nodiscard]] std::filesystem::path get_config_directory() const { return project_directory / properties.config_directory; }

    [[nodiscard]] const std::filesystem::path& get_engine_resource_directory() const { return engine_resources_path; }
    [[nodiscard]] const std::filesystem::path& get_engine_config_directory() const { return engine_config_path; }

    [[nodiscard]] ResourceDatabase& get_resource_database() { return resource_database; }
    [[nodiscard]] const ResourceDatabase& get_resource_database() const { return resource_database; }

private:
    Project(ProjectType type, ProjectProperties project_properties, std::filesystem::path working_directory, ProjectSettings&& settings);

private:
    ProjectType type;
    ProjectProperties properties;
    std::filesystem::path project_directory;
    ProjectSettings settings;

    ResourceDatabaseFacade resource_database;
    std::filesystem::path engine_resources_path;
    std::filesystem::path engine_config_path;
};
} // portal
