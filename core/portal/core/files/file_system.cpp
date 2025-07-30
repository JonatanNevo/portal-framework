//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "file_system.h"

#include <fstream>

#include "portal/core/debug/assert.h"

namespace portal
{
std::filesystem::path FileSystem::get_working_directory()
{
    return std::filesystem::current_path();
}

void FileSystem::set_working_directory(const std::filesystem::path& path)
{
    std::filesystem::current_path(path);
}

bool FileSystem::create_directory(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto abs_path = std::filesystem::absolute(path);
    std::filesystem::create_directory(abs_path, ec);
    if (ec)
        LOG_ERROR_TAG("Filesystem", "{}: Failed to create directory: {}", abs_path.string(), ec.message());
    return !ec;
}

bool FileSystem::create_directory(const std::string& file_path)
{
    return create_directory(std::filesystem::path(file_path));
}

bool FileSystem::exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

bool FileSystem::exists(const std::string& file_path)
{
    return exists(std::filesystem::path(file_path));
}

bool FileSystem::remove(const std::filesystem::path& path)
{
    if (!FileSystem::exists(path))
        return false;

    std::error_code ec;
    if (std::filesystem::is_directory(path))
        std::filesystem::remove_all(path, ec);
    else
        std::filesystem::remove(path, ec);

    if (ec)
        LOG_ERROR_TAG("Filesystem", "{}: Failed to remove file: {}", path.string(), ec.message());
    return !ec;
}

bool FileSystem::move(const std::filesystem::path& from, const std::filesystem::path& to)
{
    if (FileSystem::exists(to))
        return false;

    std::error_code ec;
    std::filesystem::rename(from, to, ec);

    if (ec)
        LOG_ERROR_TAG("Filesystem", "{}: Failed to move file: {}", from.string(), ec.message());
    return !ec;
}

bool FileSystem::copy(const std::filesystem::path& from, const std::filesystem::path& to)
{
    if (FileSystem::exists(to))
        return false;

    std::error_code ec;
    std::filesystem::copy(from, to, ec);

    if (ec)
        LOG_ERROR_TAG("Filesystem", "{}: Failed to copy file: {}", from.string(), ec.message());
    return !ec;
}

bool FileSystem::rename(const std::filesystem::path& from, const std::filesystem::path& to)
{
    return move(from, to);
}

bool FileSystem::rename_filename(const std::filesystem::path& path, const std::string& new_name)
{
    const std::filesystem::path new_path = path.parent_path() / std::filesystem::path(new_name + path.extension().string());
    return rename(path, new_path);
}

FileStat FileSystem::stat_file(const std::filesystem::path& path)
{
    std::error_code ec;

    const auto fs_start = std::filesystem::status(path, ec);
    if (ec)
    {
        LOG_TRACE_TAG("Filesystem", "{}: Failed to stat file: {}", path.string(), ec.message());
        return FileStat(
            false,
            false,
            0
        );
    }

    auto size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        LOG_WARN_TAG("Filesystem", "{}: Failed to get file size: {}", path.string(), ec.message());
        size = 0;
    }

    return FileStat(
        std::filesystem::is_regular_file(fs_start),
        std::filesystem::is_directory(fs_start),
        std::filesystem::last_write_time(path).time_since_epoch().count(),
        size
    );
}

bool FileSystem::is_file(const std::filesystem::path& path)
{
    const auto stat = stat_file(path);
    return stat.is_file;
}

bool FileSystem::is_directory(const std::filesystem::path& path)
{
    const auto stat = stat_file(path);
    return stat.is_directory;
}

bool FileSystem::is_newer(const std::filesystem::path& path_a, const std::filesystem::path& path_b)
{
    return std::filesystem::last_write_time(path_a) > std::filesystem::last_write_time(path_b);
}

uint64_t FileSystem::get_last_write_time(const std::filesystem::path& path)
{
    if (path.filename().empty() || !FileSystem::exists(path))
        return 0;

    const auto stat = stat_file(path);
    return stat.last_write_time;
}

std::filesystem::path FileSystem::get_unique_file_name(const std::filesystem::path& path)
{
    if (!FileSystem::exists(path))
        return path;

    int counter = 0;
    while (true)
    {
        ++counter;
        const std::string counter_str = [&counter]
        {
            if (counter < 10)
                return "0" + std::to_string(counter);
            else
                return std::to_string(counter);
        }();

        std::string new_file_name = std::format("{} ({})", path.stem().string(), counter_str);

        if (path.has_extension())
            new_file_name = std::format("{}{}", new_file_name, path.extension().string());

        if (!FileSystem::exists(path.parent_path() / new_file_name))
            return path.parent_path() / new_file_name;
    }
}

bool FileSystem::write_file(const std::filesystem::path& path, const Buffer& buffer)
{
    const auto abs_path = std::filesystem::absolute(path);
    // create directory if it doesn't exist
    const auto parent = abs_path.parent_path();
    if (!std::filesystem::exists(parent))
        create_directory(parent);

    std::ofstream file(abs_path, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        LOG_ERROR_TAG("Filesystem", "{}: Failed to open file for writing", abs_path.string());
        return false;
    }

    file.write(static_cast<const char*>(buffer.data), static_cast<std::streamsize>(buffer.size));
    file.close();

    return true;
}

bool FileSystem::write_file(const std::filesystem::path& path, const std::string& data)
{
    return write_file(path, Buffer(const_cast<void*>(static_cast<const void*>(data.data())), data.size()));
}

bool FileSystem::write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data)
{
    return write_file(path, Buffer(const_cast<void*>(static_cast<const void*>(data.data())), data.size()));
}

Buffer FileSystem::read_chunk(const std::filesystem::path& path, size_t offset, size_t count)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOG_ERROR_TAG("Filesystem", "{}: Failed to open file for reading", std::filesystem::absolute(path).string());
        return Buffer{};
    }

    auto size = stat_file(path).size;
    if (offset + count > size)
    {
        LOG_WARN_TAG("Filesystem", "{}: Requested read chunk ({} + {}) is bigger than size: ({})", path.string(), offset, count, size);
        return Buffer{};
    }

    Buffer&& buffer = Buffer::allocate(count);
    file.seekg(static_cast<std::streamsize>(offset), std::ios::beg);
    file.read(buffer.as<char*>(), static_cast<std::streamsize>(count));
    return std::move(buffer);
}

Buffer FileSystem::read_file_binary(const std::filesystem::path& path)
{
    const auto stat = stat_file(path);
    return read_chunk(path, 0, stat.size);
}


std::string FileSystem::read_file_string(const std::filesystem::path& path)
{
    auto binary = read_file_binary(path);
    return {binary.as<const char*>(), binary.size};
}

FileStatus FileSystem::try_open_file(const std::filesystem::path& path)
{
    // First check if file exists
    if (!std::filesystem::exists(path))
        return FileStatus::Invalid;

    // Try to open the file for reading
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        // File exists but couldn't be opened - likely locked or permission issue
        return FileStatus::Locked;
    }

    // Test if we can actually read from it
    char buffer;
    if (file.read(&buffer, 0).fail() && !file.eof())
    {
        file.close();
        return FileStatus::OtherError;
    }

    file.close();
    return FileStatus::Success;
}

FileStatus FileSystem::try_open_file_and_wait(const std::filesystem::path& path, uint64_t wait_ms)
{
    const auto status = try_open_file(path);
    if (status == FileStatus::Locked)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        return try_open_file(path);
    }
    return status;
}

// TODO: Implement these

std::filesystem::path FileSystem::open_file_dialog(const std::initializer_list<FileDialogFilterItem>)
{
    return {};
}

std::filesystem::path FileSystem::open_folder_dialog(const char*)
{
    return {};
}

std::filesystem::path FileSystem::save_file_dialog(const std::initializer_list<FileDialogFilterItem>)
{
    return {};
}
} // portal
