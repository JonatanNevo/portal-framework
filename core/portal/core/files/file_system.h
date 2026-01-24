//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/core/buffer.h"

namespace portal
{
enum class FileStatus
{
    Success    = 0,
    Invalid    = 1,
    Locked     = 2,
    OtherError = 3
};

struct FileStat
{
    bool is_file;
    bool is_directory;
    uint64_t last_write_time;
    size_t size;
};

class FileSystem final
{
public:
    struct FileDialogFilterItem
    {
        const char* name;
        const char* spec;
    };

public:
    static std::filesystem::path get_working_directory();
    static void set_working_directory(const std::filesystem::path& path);

    static bool create_directory(const std::filesystem::path& path);
    static bool create_directory(const std::string& file_path);

    static bool exists(const std::filesystem::path& path);
    static bool exists(const std::string& file_path);

    static bool remove(const std::filesystem::path& path);

    static bool move(const std::filesystem::path& from, const std::filesystem::path& to);
    static bool copy(const std::filesystem::path& from, const std::filesystem::path& to);
    static bool rename(const std::filesystem::path& from, const std::filesystem::path& to);
    static bool rename_filename(const std::filesystem::path& path, const std::string& new_name);

    static FileStat stat_file(const std::filesystem::path& path);
    static bool is_file(const std::filesystem::path& path);
    static bool is_directory(const std::filesystem::path& path);

    static bool is_newer(const std::filesystem::path& path_a, const std::filesystem::path& path_b);
    static uint64_t get_last_write_time(const std::filesystem::path& path);

    static std::filesystem::path get_unique_file_name(const std::filesystem::path& path);

    static bool write_file(const std::filesystem::path& path, const Buffer& buffer, size_t offset = 0);
    static bool write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data, size_t offset = 0);
    static bool write_file(const std::filesystem::path& path, const std::string& data, size_t offset = 0);

    static Buffer read_chunk(const std::filesystem::path& path, size_t offset, size_t count);
    static Buffer read_file_binary(const std::filesystem::path& path);
    static std::string read_file_string(const std::filesystem::path& path);

    static FileStatus try_open_file(const std::filesystem::path& path);
    static FileStatus try_open_file_and_wait(const std::filesystem::path& path, uint64_t wait_ms = 100);

    static std::filesystem::path open_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters = {});
    static std::filesystem::path open_folder_dialog(const char* initial_folder = "");
    static std::filesystem::path save_file_dialog(std::initializer_list<FileDialogFilterItem> in_filters = {});

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Platform Specific Functions ////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static bool show_file_in_explorer(const std::filesystem::path& path);
    static bool open_directory_in_explorer(const std::filesystem::path& path);
    static bool open_externally(const std::filesystem::path& path);

    static bool has_environment_variable(const std::string& name);
    static bool set_environment_variable(const std::string& name, const std::string& value);
    static std::string get_environment_variable(const std::string& name);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Program Data Paths /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static std::filesystem::path get_binary_path();
    static std::filesystem::path get_resource_path();
    static std::filesystem::path get_root_path();

    /**
     * Retrieves the base folder for storing data files.
     *
     * On Windows this defaults to %APPDATA% (Roaming profile)
     * On Linux this defaults to ~/.local/share but can be configured by the user
     *
     * @return The base folder for storing program data.
     */
    static std::filesystem::path get_data_home(std::filesystem::path app_name = "");


    /**
     * Retrieves the base folder for storing config files.
     *
     * On Windows this defaults to %APPDATA% (Roaming profile)
     * On Linux this defaults to ~/.config but can be configured by the user
     *
     * @return The base folder for storing config data.
     */
    static std::filesystem::path get_config_home(std::filesystem::path app_name = "");

    /**
     * Retrieves the base folder for storing cache files.
     *
     * On Windows this defaults to %APPDATALOCAL%
     * On Linux this defaults to ~/.cache but can be configured by the user
     *
     * Note that it is recommended to append "cache" after the program name to prevent conflicting with "StateDir" under Windows
     * @return The base folder for storing data that do not need to be backed up and might be deleted.
     */
    static std::filesystem::path get_cache_dir(std::filesystem::path app_name = "");

    /**
     * Retrieves the base folder used for state files.
     *
     * On Windows this defaults to %APPDATALOCAL%
     * On Linux this defaults to ~/.local/state but can be configured by the user
     * On OS X this is the same as getDataHome()
     *
     * @return The base folder for storing data that do not need to be backed up but should not be regularly deleted either.
     */
    static std::filesystem::path get_state_dir(std::filesystem::path app_name = "");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////// Global Platform Paths //////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * The folder that represents the desktop.
     * Normally you should try not to use this folder.
     *
     * @return Absolute path to the user's desktop
     */
    static std::filesystem::path get_desktop_folder();

    /**
     * The folder to store user documents to
     *
     * @return Absolute path to the "Documents" folder
     */
    static std::filesystem::path get_documents_folder();

    /**
     * The folder where files are downloaded.
     *
     * @return Absolute path to the folder where files are downloaded to.
     */
    static std::filesystem::path get_download_folder();

    /**
     * The folder for storing the user's pictures.
     *
     * @return Absolute path to the "Picture" folder
     */
    static std::filesystem::path get_pictures_folder();

    /**
     * This returns the folder that can be used for sharing files with other users on the same system.
     *
     * @return Absolute path to the "Public" folder
     */
    static std::filesystem::path get_public_folder();

    /**
     * The folder where music is stored
     *
     * @return Absolute path to the music folder
     */
    static std::filesystem::path get_music_folder();

    /**
     * The folder where video is stored
     *
     * @return Absolute path to the video folder
     */
    static std::filesystem::path get_video_folder();
};
} // portal
