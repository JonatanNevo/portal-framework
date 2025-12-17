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
    explicit FileSource(std::filesystem::path path);

    [[nodiscard]] Buffer load() const override;
    [[nodiscard]] Buffer load(size_t offset, size_t size) const override;
    [[nodiscard]] std::unique_ptr<std::istream> stream() const override;

protected:
    std::filesystem::path file_path;
};
} // portal
