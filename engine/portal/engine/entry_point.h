//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file entry_point.h
 * @brief Portal Engine application entry point.
 *
 * This header provides the main entry point for Portal Engine Users.
 * This is a wrapper around <portal/application/entry_point.h> with additional checks and actions
 * That only 3D and gui applications require
 *
 * Example usage:

 * @code
 * #include <portal/application/entry_point.h>
 *
 * std::unique_ptr<portal::Application> portal::create_application(int argc, char** argv) {
 *     ApplicationProperties props{.name = STRING_ID("My Game")};
 *     return std::make_unique<MyGameApp>(props);
 * }
 * @endcode
 */
#pragma once

#include <argparse/argparse.hpp>
#include <portal/application/entry_point.h>

#include <portal/engine/reference.h>
#include "project/project.h"


namespace portal
{
extern std::unique_ptr<Application> create_engine_application(Reference<Project>&& project, int argc, char** argv);

inline ApplicationProperties from_project(Project& project)
{
    return ApplicationProperties{
        .name = project.get_name(),
        .width = project.get_settings().get_setting<size_t>("application.window.width", 1600),
        .height = project.get_settings().get_setting<size_t>("application.window.height", 900),
    };
}

inline std::unique_ptr<Application> create_application(int argc, char** argv)
{
#ifdef PORTAL_BUILD_EDITOR
    // TODO: open project based on args or environment variables or something idk
    argparse::ArgumentParser parser("portal-engine", PORTAL_ENGINE_VERSION);
    parser.add_argument("-p", "--project")
          .help("Path to the project folder")
          .default_value(FileSystem::get_working_directory().string());

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        LOG_ERROR("Error in parsing arguments: {}", err.what());
        std::exit(1);
    }

    auto working_directory = std::filesystem::absolute(parser.get<std::string>("-p"));
    auto project = Project::open_project(ProjectType::Editor, working_directory);
#else
    // TODO: embed project settings into executable
    auto project = Project::open_project(ProjectType::Runtime, FileSystem::get_working_directory());
#endif

    return create_engine_application(std::move(project), argc, argv);
}
}
