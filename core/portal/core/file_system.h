//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include <filesystem>

namespace portal::filesystem
{
struct FileStat
{
    bool is_file;
    bool is_directory;
    size_t size;
};

class FileSystem final
{
public:
    virtual ~FileSystem() = default;
    virtual FileStat stat_file(const std::filesystem::path& path);
    virtual bool is_file(const std::filesystem::path& path);
    virtual bool is_directory(const std::filesystem::path& path);
    virtual bool exists(const std::filesystem::path& path);
    virtual bool create_directory(const std::filesystem::path& path);
    virtual std::vector<uint8_t> read_chunk(const std::filesystem::path& path, size_t offset, size_t count);
    virtual void write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);
    virtual void remove(const std::filesystem::path& path);
    virtual void write_file(const std::filesystem::path& path, const std::string& data);

    // Read the entire file into a string
    virtual std::string read_file_string(const std::filesystem::path& path);

    // Read the entire file into a vector of bytes
    virtual std::vector<uint8_t> read_file_binary(const std::filesystem::path& path);
};

void init();
std::shared_ptr<FileSystem> get();

// wrapper around all static functions
FileStat stat_file(const std::filesystem::path& path);
bool is_file(const std::filesystem::path& path);
bool is_directory(const std::filesystem::path& path);
bool exists(const std::filesystem::path& path);
bool create_directory(const std::filesystem::path& path);
std::vector<uint8_t> read_chunk(const std::filesystem::path& path, size_t offset, size_t count);
void write_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);
void remove(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, const std::string& data);
std::string read_file_string(const std::filesystem::path& path);
std::vector<uint8_t> read_file_binary(const std::filesystem::path& path);

std::string get_file_extension(const std::filesystem::path& path);
} // portal
