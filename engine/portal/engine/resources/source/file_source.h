//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "resource_source.h"

namespace portal::resources
{

class FileSource final : public ResourceSource
{
public:
    explicit FileSource(const std::filesystem::path& path);
    [[nodiscard]] SourceMetadata get_meta() const override;
    Buffer load() override;
    Buffer load(size_t offset, size_t size) override;
    std::unique_ptr<std::istream> stream() override;

protected:
    std::filesystem::path file_path;
};

} // portal
