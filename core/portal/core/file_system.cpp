//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "file_system.h"

#include "portal/core/assert.h"

namespace portal::filesystem
{
static std::shared_ptr<FileSystem> fs = nullptr;

void init()
{
    fs = std::make_shared<FileSystem>();
}

std::shared_ptr<FileSystem> get()
{
    PORTAL_CORE_ASSERT(fs, "Filesystem not initialized");
    return fs;
}

void FileSystem::write_file(const std::filesystem::path& path, const std::string& data)
{
    write_file(path, std::vector<uint8_t>(data.begin(), data.end()));
}

std::string FileSystem::read_file_string(const std::filesystem::path& path)
{
    auto binary = read_file_binary(path);
    return {binary.begin(), binary.end()};
}

std::vector<uint8_t> FileSystem::read_file_binary(const std::filesystem::path& path)
{
    const auto stat = stat_file(path);
    return read_chunk(path, 0, stat.size);
}

FileStat FileSystem::stat_file(const std::filesystem::path& path)
{
    std::error_code ec;

    const auto fs_start = std::filesystem::status(path, ec);
    if (ec)
    {
        LOG_CORE_TRACE_TAG("Filesystem", "{}: Failed to stat file: {}", path.string(), ec.message());
        return FileStat(
            false,
            false,
            0
        );
    }

    auto size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        LOG_CORE_WARN_TAG("Filesystem", "{}: Failed to get file size: {}", path.string(), ec.message());
        size = 0;
    }

    return FileStat(
        std::filesystem::is_regular_file(fs_start),
        std::filesystem::is_directory(fs_start),
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

bool FileSystem::exists(const std::filesystem::path& path)
{
    return std::filesystem::exists(path);
}

bool FileSystem::create_directory(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::create_directory(path, ec);
    if (ec)
        LOG_CORE_ERROR_TAG("Filesystem", "{}: Failed to create directory: {}", path.string(), ec.message());
    return !ec;
}

std::vector<uint8_t> FileSystem::read_chunk(const std::filesystem::path& path, size_t offset, size_t count)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOG_CORE_ERROR_TAG("Filesystem", "{}: Failed to open file for reading", path.string());
        return {};
    }

    auto size = stat_file(path).size;
    if (offset + count > size)
    {
        LOG_CORE_WARN_TAG("Filesystem", "{}: Requested read chunk ({} + {}) is bigger than size: ({})", path.string(), offset, count, size);
        return {};
    }

    file.seekg(offset, std::ios::beg);
    std::vector<uint8_t> data(count);
    file.read(reinterpret_cast<char*>(data.data()), count);
    return data;
}

void FileSystem::write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data)
{
    // create directory if it doesn't exist
    const auto parent = path.parent_path();
    if (!std::filesystem::exists(parent))
        create_directory(parent);

    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        LOG_CORE_ERROR_TAG("Filesystem", "{}: Failed to open file for writing", path.string());
        return;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void FileSystem::remove(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::remove_all(path, ec);

    if (ec)
        LOG_CORE_ERROR_TAG("Filesystem", "{}: Failed to remove file: {}", path.string(), ec.message());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FileStat stat_file(const std::filesystem::path& path) { return get()->stat_file(path); }

bool is_file(const std::filesystem::path& path) { return get()->is_file(path); }

bool is_directory(const std::filesystem::path& path) { return get()->is_directory(path); }

bool exists(const std::filesystem::path& path) { return get()->exists(path); }

bool create_directory(const std::filesystem::path& path) { return get()->create_directory(path); }

std::vector<uint8_t> read_chunk(const std::filesystem::path& path, const size_t offset, const size_t count)
{
    return get()->read_chunk(path, offset, count);
}

void write_file(const std::filesystem::path& path, std::vector<uint8_t>& data) { return get()->write_file(path, data); }

void remove(const std::filesystem::path& path) { get()->remove(path); }

void write_file(const std::filesystem::path& path, const std::string& data) { get()->write_file(path, data); }

std::string read_file_string(const std::filesystem::path& path) { return get()->read_file_string(path); }

std::vector<uint8_t> read_file_binary(const std::filesystem::path& path) { return get()->read_file_binary(path); }

std::string get_file_extension(const std::filesystem::path& path)
{
    return path.extension().string();
}
} // portal
