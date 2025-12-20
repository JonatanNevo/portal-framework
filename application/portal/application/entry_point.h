//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

/**
 * @file entry_point.h
 * @brief Portal Framework application entry point.
 *
 * This header provides the main entry point for Portal applications. Users include
 * this file in their main.cpp and implement create_application() to instantiate
 * their Application-derived class. The framework handles logging initialization,
 * exception handling, and platform-specific entry points (WinMain on Windows).
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

#include <portal/application/application.h>

extern std::unique_ptr<portal::Application> portal::create_application(int arc, char** argv);

namespace portal
{
/**
 * Portal Framework main function.
 *
 * Initializes logging, creates the application via create_application(), runs the
 * main loop, and handles top-level exceptions. Called by the platform-specific
 * entry point (main or WinMain).
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success)
 */
int main(int argc, char** argv)
{
    // TODO: fetch default log levels from environment?
    Log::init({.default_log_level = Log::LogLevel::Trace});

    auto application = create_application(argc, argv);
    try
    {
        application->run();
    }
    catch (std::exception& e)
    {
        LOG_FATAL("Unhandled exception: {}", e.what());
    }
    catch (...)
    {
        LOG_FATAL("Unhandled unknown exception");
    }
    application.reset();

    Log::shutdown();
    return 0;
}
}

#if defined(PORTAL_PLATFORM_WINDOWS) && defined(PORTAL_DIST)

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    return portal::main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
    return portal::main(argc, argv);
}

#endif
