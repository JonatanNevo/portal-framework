//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "file_source.h"

#include <fstream>
#include <utility>

#include "portal/core/files/file_system.h"

namespace portal::resources
{

static auto logger = Log::get_logger("Resources");

FileSource::FileSource(std::filesystem::path path) : file_path(std::move(path)) {}

Buffer FileSource::load() const
{
    if (!std::filesystem::exists(file_path))
    {
        LOGGER_ERROR("Path for resource does not exist: {}", file_path.string());
        return {};
    }

    return FileSystem::read_file_binary(file_path);
}

Buffer FileSource::load(const size_t offset, const size_t size) const
{
    if (!std::filesystem::exists(file_path))
    {
        LOGGER_ERROR("Path for resource does not exist: {}", file_path.string());
        return {};
    }

    return FileSystem::read_chunk(file_path, offset, size);
}

std::unique_ptr<std::istream> FileSource::stream() const
{
    if (!std::filesystem::exists(file_path))
    {
        LOGGER_ERROR("Path for resource does not exist: {}", file_path.string());
        return std::make_unique<std::ifstream>();
    }

    auto&& file = std::make_unique<std::ifstream>(file_path, std::ios::binary);
    return file;
}

} // portal
