//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include <filesystem>

#include "portal/core/buffer.h"

namespace portal
{

enum class FileStatus
{
    Success = 0,
    Invalid = 1,
    Locked = 2,
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

    static bool write_file(const std::filesystem::path& path, const Buffer& buffer);
    static bool write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);
    static bool write_file(const std::filesystem::path& path, const std::string& data);

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

    static std::filesystem::path get_persistent_storage_path();

    static bool has_environment_variable(const std::string& name);
    static bool set_environment_variable(const std::string& name, const std::string& value);
    static std::string get_environment_variable(const std::string& name);
};
} // portal
