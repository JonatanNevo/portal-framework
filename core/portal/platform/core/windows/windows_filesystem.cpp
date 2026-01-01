//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/files/file_system.h"

#include <Windows.h>
#include <Shlobj.h>
#include <stringapiset.h>

namespace portal
{
static std::filesystem::path s_program_path;


static std::string utf16_to_utf8(const wchar_t* w_str)
{
    std::string res;

    // If the 6th parameter is 0 then WideCharToMultiByte returns the number of bytes needed to store the result.
    int size = WideCharToMultiByte(CP_UTF8, 0, w_str, -1, nullptr, 0, nullptr, nullptr);
    if (size > 0)
    {
        //If the converted UTF-8 string could not be in the initial buffer. Allocate one that can hold it.
        std::vector<char> buffer(size);
        size = WideCharToMultiByte(CP_UTF8, 0, w_str, -1, &buffer[0], static_cast<int>(buffer.size()), nullptr, nullptr);
        res = buffer.data();
    }
    if (size == 0)
    {
        // WideCharToMultiByte return 0 for errors.
        throw std::runtime_error("UTF16 to UTF8 failed with error code: " + std::to_string(GetLastError()));
    }
    return res;
}

class FreeCoTaskMemory
{
    LPWSTR pointer = nullptr;

public:
    explicit FreeCoTaskMemory(const LPWSTR pointer) : pointer(pointer) {};

    ~FreeCoTaskMemory()
    {
        CoTaskMemFree(pointer);
    }
};

static std::filesystem::path get_known_windows_folder(REFKNOWNFOLDERID folder_id, const char* error_msg)
{
    LPWSTR windows_path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_CREATE, nullptr, &windows_path);
    FreeCoTaskMemory scopeBoundMemory(windows_path);

    if (!SUCCEEDED(result))
    {
        throw std::runtime_error(error_msg);
    }

    return utf16_to_utf8(windows_path);
}

static std::filesystem::path get_appdata()
{
    return get_known_windows_folder(FOLDERID_RoamingAppData, "RoamingAppData could not be found");
}

static std::filesystem::path get_appdata_local()
{
    return get_known_windows_folder(FOLDERID_LocalAppData, "RoamingAppData could not be found");
}

bool FileSystem::show_file_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    const std::string cmd = fmt::format("explorer.exe /select,\"{}\"", absolute_path.string());
    system(cmd.c_str());
    return true;
}

bool FileSystem::open_directory_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    ShellExecute(nullptr, reinterpret_cast<LPCSTR>(L"explore"), reinterpret_cast<LPCSTR>(absolute_path.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

bool FileSystem::open_externally(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    ShellExecute(nullptr, reinterpret_cast<LPCSTR>(L"open"), reinterpret_cast<LPCSTR>(absolute_path.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

bool FileSystem::has_environment_variable(const std::string& name)
{
    HKEY hKey;
    LSTATUS lOpenStatus = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_ALL_ACCESS, &hKey);

    if (lOpenStatus == ERROR_SUCCESS)
    {
        lOpenStatus = RegQueryValueExA(hKey, name.c_str(), 0, NULL, NULL, NULL);
        RegCloseKey(hKey);
    }

    return lOpenStatus == ERROR_SUCCESS;
}

bool FileSystem::set_environment_variable(const std::string& name, const std::string& value)
{
    HKEY hKey;
    const auto key_path = "Environment";
    DWORD create_new_key;
    const LSTATUS lOpenStatus = RegCreateKeyExA(
        HKEY_CURRENT_USER,
        key_path,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        nullptr,
        &hKey,
        &create_new_key
    );
    if (lOpenStatus == ERROR_SUCCESS)
    {
        const LSTATUS lSetStatus = RegSetValueExA(hKey, name.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), static_cast<DWORD>(value.length() + 1));
        RegCloseKey(hKey);

        if (lSetStatus == ERROR_SUCCESS)
        {
            SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>("Environment"), SMTO_BLOCK, 100, nullptr);
            return true;
        }
    }

    return false;
}

std::string FileSystem::get_environment_variable(const std::string& name)
{
    char* value = nullptr;
    size_t len = 0;

    if (_dupenv_s(&value, &len, name.c_str()) == 0 && value != nullptr)
    {
        std::string result(value);
        free(value); // Free the allocated memory
        return result;
    }
    return {};
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

void FileSystem::set_program_path(std::filesystem::path program_path)
{
    PORTAL_ASSERT(program_path.is_relative(), "Program path must be relative");
    s_program_path = std::move(program_path);
}

std::filesystem::path FileSystem::get_data_home()
{
    return get_appdata() / s_program_path;
}

std::filesystem::path FileSystem::get_config_home()
{
    return get_appdata() / s_program_path / "config";
}

std::filesystem::path FileSystem::get_cache_dir()
{
    return get_appdata_local() / s_program_path / "cache";
}

std::filesystem::path FileSystem::get_state_dir()
{
    return get_appdata_local() / s_program_path;
}

std::filesystem::path FileSystem::get_desktop_folder()
{
    return get_known_windows_folder(FOLDERID_Desktop, "Desktop folder could not be found");
}

std::filesystem::path FileSystem::get_documents_folder()
{
    return get_known_windows_folder(FOLDERID_Documents, "Documents folder could not be found");
}

std::filesystem::path FileSystem::get_download_folder()
{
    return get_known_windows_folder(FOLDERID_Downloads, "Downloads folder could not be found");
}

std::filesystem::path FileSystem::get_pictures_folder()
{
    return get_known_windows_folder(FOLDERID_Pictures, "Pictures folder could not be found");
}

std::filesystem::path FileSystem::get_public_folder()
{
    return get_known_windows_folder(FOLDERID_Public, "Public folder could not be found");
}

std::filesystem::path FileSystem::get_music_folder()
{
    return get_known_windows_folder(FOLDERID_Music, "Music folder could not be found");
}

std::filesystem::path FileSystem::get_video_folder()
{
    return get_known_windows_folder(FOLDERID_Videos, "Videos folder could not be found");
}
}
