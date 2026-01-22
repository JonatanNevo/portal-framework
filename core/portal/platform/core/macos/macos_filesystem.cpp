//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/files/file_system.h"

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>

namespace portal
{
static CFBundleRef s_bundle_instance;

static std::filesystem::path get_home()
{
    std::filesystem::path res;

    const unsigned int uid = getuid();
    const char* home_env = std::getenv("HOME");
    if (uid != 0 && home_env)
    {
        // Ignore "HOME" if root
        res = home_env;
        return res;
    }

    passwd* pw = nullptr;
    passwd pwd{};

    long buffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buffer_size < 1)
    {
        buffer_size = 16384;
    }

    std::vector<char> buffer(buffer_size);
    int error_code = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw);
    while (error_code == ERANGE)
    {
        // Buffer was too small
        buffer_size *= 2;
        buffer.resize(buffer_size);
        error_code = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw);
    }

    if (error_code)
        throw std::runtime_error("Failed to get passwd struct");

    const char* temp_res = pw->pw_dir;
    if (!temp_res)
        throw std::runtime_error("User has no home directory");

    res = temp_res;
    return res;
}

static auto get_bundle()
{
    if (!s_bundle_instance)
        s_bundle_instance = CFBundleGetMainBundle();
    return s_bundle_instance;
}

bool FileSystem::show_file_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    // -R flag reveals the file in Finder
    const std::string cmd = "open -R \"" + absolute_path.string() + "\"";
    return system(cmd.c_str()) == 0;
}

bool FileSystem::open_directory_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    const std::string cmd = "open \"" + absolute_path.string() + "\"";
    return system(cmd.c_str()) == 0;
}

bool FileSystem::open_externally(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    const std::string cmd = "open \"" + absolute_path.string() + "\"";
    return system(cmd.c_str()) == 0;
}

bool FileSystem::has_environment_variable(const std::string& name)
{
    return std::getenv(name.c_str()) != nullptr;
}

bool FileSystem::set_environment_variable(const std::string& name, const std::string& value)
{
    // setenv returns 0 on success
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

std::string FileSystem::get_environment_variable(const std::string& name)
{
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : std::string{};
}

std::filesystem::path FileSystem::get_binary_path()
{
    const CFURLRef executable_url = CFBundleCopyExecutableURL(get_bundle());
    if (!executable_url)
        throw std::runtime_error("Failed to get executable URL");

    char executable_path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(executable_url, TRUE, reinterpret_cast<UInt8*>(executable_path), PATH_MAX))
    {
        CFRelease(executable_url);
        throw std::runtime_error("Failed to get executable path");
    }
    CFRelease(executable_url);
    return std::filesystem::path(executable_path).parent_path();
}

std::filesystem::path FileSystem::get_resource_path()
{
    const CFURLRef resource_url = CFBundleCopyResourcesDirectoryURL(get_bundle());
    char resource_path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resource_url, TRUE, reinterpret_cast<UInt8*>(resource_path), PATH_MAX))
    {
        CFRelease(resource_url);
        throw std::runtime_error("Failed to get resource path");
    }
    CFRelease(resource_url);
    return resource_path;
}

std::filesystem::path FileSystem::get_root_path()
{
    const CFURLRef bundle_url = CFBundleCopyBundleURL(get_bundle());
    if (!bundle_url)
        throw std::runtime_error("Failed to get bundle URL");

    char bundle_path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(bundle_url, TRUE, reinterpret_cast<UInt8*>(bundle_path), PATH_MAX))
    {
        CFRelease(bundle_url);
        throw std::runtime_error("Failed to get bundle path");
    }
    CFRelease(bundle_url);
    return bundle_path;
}

std::filesystem::path FileSystem::get_data_home(const std::filesystem::path& app_name)
{
    return get_home() / "Library" / "Application Support" / app_name;
}

std::filesystem::path FileSystem::get_config_home(const std::filesystem::path& app_name)
{
    return get_home() / "Library" / "Application Support" / app_name / "config";
}

std::filesystem::path FileSystem::get_cache_dir(const std::filesystem::path app_name)
{
    return get_home() / "Library" / "Caches" / app_name;
}

std::filesystem::path FileSystem::get_state_dir(const std::filesystem::path app_name)
{
    return get_home() / "Library" / "Application Support" / app_name;
}

std::filesystem::path FileSystem::get_desktop_folder()
{
    return get_home() / "Desktop";
}

std::filesystem::path FileSystem::get_documents_folder()
{
    return get_home() / "Documents";
}

std::filesystem::path FileSystem::get_download_folder()
{
    return get_home() / "Downloads";
}

std::filesystem::path FileSystem::get_pictures_folder()
{
    return get_home() / "Pictures";
}

std::filesystem::path FileSystem::get_public_folder()
{
    return get_home() / "Public";
}

std::filesystem::path FileSystem::get_music_folder()
{
    return get_home() / "Music";
}

std::filesystem::path FileSystem::get_video_folder()
{
    return get_home() / "Movies";
}
}
