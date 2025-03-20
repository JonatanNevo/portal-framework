//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once
#include <string>
#include <vector>

namespace portal
{
class UnixPlatformContext;
class WindowsPlatformContext;

/**
 * @brief A platform context contains abstract platform specific operations
 *
 *        A platform can be thought as the physical device and operating system configuration that the application
 *        is running on.
 *
 *        Some platforms can be reused across different hardware configurations, such as Linux and Macos as both
 *        are POSIX compliant. However, some platforms are more specific such as Android and Windows
 */
class PlatformContext
{
    // only allow platform contexts to be created by the platform specific implementations
    friend class UnixPlatformContext;
    friend class WindowsPlatformContext;

public:
    virtual ~PlatformContext() = default;

    [[nodiscard]] virtual const std::vector<std::string>& arguments() const { return _arguments; }
    [[nodiscard]] virtual const std::string& external_storage_directory() const { return _external_storage_directory; }
    [[nodiscard]] virtual const std::string& temp_directory() const { return _temp_directory; }

protected:
    std::vector<std::string> _arguments;
    std::string _external_storage_directory;
    std::string _temp_directory;

    PlatformContext() = default;
};
}
