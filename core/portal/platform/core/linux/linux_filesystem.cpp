//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/files/file_system.h"

#include <fmt/std.h>

#include <cstdlib>
#include <unordered_map>
#include <fstream>
#include <sys/types.h>

#include <cstring>
#include <pwd.h>
#include <ranges>
#include <sstream>


namespace portal
{
static void add_to_platform_folders_form_file(std::filesystem::path filename, std::unordered_map<std::string, std::filesystem::path>& folders)
{
    std::ifstream infile(filename.c_str());
    std::string line;
    while (std::getline(infile, line))
    {
        if (line.length() == 0 || line.at(0) == '#' || line.substr(0, 4) != "XDG_" || line.find("_DIR") == std::string::npos)
        {
            continue;
        }
        try
        {
            std::size_t splitPos = line.find('=');
            std::string key = line.substr(0, splitPos);
            std::size_t valueStart = line.find('"', splitPos);
            std::size_t valueEnd = line.find('"', valueStart + 1);
            std::string value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
            folders[key] = value;
        }
        catch (std::exception& e)
        {
            LOG_WARN_TAG("Filesystem", "WARNING: Failed to process \"{}\" from \"{}\". Error: {}", line, filename, e.what());
        }
    }
}

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

static std::unordered_map<std::string, std::filesystem::path>& get_platform_folders()
{
    static std::unordered_map<std::string, std::filesystem::path> folders;

    if (folders.empty())
    {
        auto home = get_home();
        folders["XDG_DOCUMENTS_DIR"] = home / "Documents";
        folders["XDG_DESKTOP_DIR"] = home / "Desktop";
        folders["XDG_DOWNLOAD_DIR"] = home / "Downloads";
        folders["XDG_MUSIC_DIR"] = home / "Music";
        folders["XDG_PICTURES_DIR"] = home / "Pictures";
        folders["XDG_PUBLICSHARE_DIR"] = home / "Public";
        folders["XDG_TEMPLATES_DIR"] = home / ".Templates";
        folders["XDG_VIDEOS_DIR"] = home / "Videos";
        add_to_platform_folders_form_file(FileSystem::get_config_home() / "user-dirs.dirs", folders);
    }

    return folders;
}

static void throw_on_relative(const char* env_name, const char* env_value)
{
    if (env_value[0] != '/')
    {
        throw std::runtime_error(
            fmt::format(
                "Environment \"{}\" does not start with an '/'. XDG specifies that the value must be absolute. The current value is: \"{}\"",
                env_name,
                env_value
            )
        );
    }
}

static std::filesystem::path get_linux_folder_default(const char* env_name, const char* default_relative_path)
{
    const char* temp_res = std::getenv(env_name);
    if (temp_res)
    {
        throw_on_relative(env_name, temp_res);
        return temp_res;
    }
    return get_home() / default_relative_path;;
}


bool FileSystem::show_file_in_explorer(const std::filesystem::path& path)
{
    // On Linux, there isn't a single standard command to "select" a file in the file manager.
    // The common fallback is to open the directory containing the file.
    if (std::filesystem::exists(path))
    {
        return open_directory_in_explorer(path.parent_path());
    }

    return false;
}

bool FileSystem::open_directory_in_explorer(const std::filesystem::path& path)
{
    return open_externally(path);
}

bool FileSystem::open_externally(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
    {
        return false;
    }

    // xdg-open is the standard way to open files/folders in the user's preferred app on Linux.
    const std::string command = "xdg-open \"" + path.string() + "\" &";
    return std::system(command.c_str()) == 0;
}

bool FileSystem::has_environment_variable(const std::string& name)
{
    return std::getenv(name.c_str()) != nullptr;
}

bool FileSystem::set_environment_variable(const std::string& name, const std::string& value)
{
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

std::string FileSystem::get_environment_variable(const std::string& name)
{
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : std::string{};
}

std::filesystem::path FileSystem::get_binary_path()
{
    return std::filesystem::current_path();
}

std::filesystem::path FileSystem::get_resource_path()
{
    return std::filesystem::current_path();
}

std::filesystem::path FileSystem::get_root_path()
{
    return std::filesystem::current_path();
}


std::filesystem::path FileSystem::get_data_home(const std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_DATA_HOME", ".local/share") / "portal" / app_name;
}

std::filesystem::path FileSystem::get_config_home(const std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_CONFIG_HOME", ".config") / "portal" / app_name;
}

std::filesystem::path FileSystem::get_cache_dir(const std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_CACHE_HOME", ".cache") / "portal" / app_name;
}

std::filesystem::path FileSystem::get_state_dir(const std::filesystem::path app_name)
{
    return get_linux_folder_default("XDG_STATE_HOME", ".local/state") / "portal" / app_name;
}

std::filesystem::path FileSystem::get_desktop_folder()
{
    return get_platform_folders()["XDG_DESKTOP_DIR"];
}

std::filesystem::path FileSystem::get_documents_folder()
{
    return get_platform_folders()["XDG_DOCUMENTS_DIR"];
}

std::filesystem::path FileSystem::get_download_folder()
{
    return get_platform_folders()["XDG_DOWNLOAD_DIR"];
}

std::filesystem::path FileSystem::get_pictures_folder()
{
    return get_platform_folders()["XDG_PICTURES_DIR"];
}

std::filesystem::path FileSystem::get_public_folder()
{
    return get_platform_folders()["XDG_PUBLICSHARE_DIR"];
}

std::filesystem::path FileSystem::get_music_folder()
{
    return get_platform_folders()["XDG_MUSIC_DIR"];
}

std::filesystem::path FileSystem::get_video_folder()
{
    return get_platform_folders()["XDG_VIDEOS_DIR"];
}
}
