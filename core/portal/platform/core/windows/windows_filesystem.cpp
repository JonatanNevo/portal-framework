//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/core/file_system.h"

#include <Windows.h>
#include <Shlobj.h>

namespace portal
{
// TODO: Move from here to asset manager
static std::filesystem::path s_persistent_storage_path;

bool FileSystem::show_file_in_explorer(const std::filesystem::path& path)
{
    const auto absolute_path = std::filesystem::canonical(path);
    if (!exists(absolute_path))
        return false;

    const std::string cmd = std::format("explorer.exe /select,\"{}\"", absolute_path.string());
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

std::filesystem::path FileSystem::get_persistent_storage_path()
{
    if (!s_persistent_storage_path.empty())
        return s_persistent_storage_path;

    PWSTR roaming_file_path;
    [[maybe_unused]] const HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &roaming_file_path);
    PORTAL_CORE_ASSERT(result == S_OK, "Failed to get Roaming App Data path");
    s_persistent_storage_path = roaming_file_path;
    s_persistent_storage_path /= "portal"; // TODO: allow multiple folders

    if (!exists(s_persistent_storage_path))
        create_directory(s_persistent_storage_path);

    return s_persistent_storage_path;
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
}
