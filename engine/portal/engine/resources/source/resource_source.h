//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"

namespace portal::resources
{
class ResourceSource
{
public:
    virtual ~ResourceSource() = default;
    [[nodiscard]] virtual Buffer load() const = 0;
    [[nodiscard]] virtual Buffer load(size_t offset, size_t size) const = 0;
    [[nodiscard]] virtual std::unique_ptr<std::istream> istream() const = 0;

    virtual void save(Buffer data, size_t offset = 0) = 0;
    [[nodiscard]] virtual std::unique_ptr<std::ostream> ostream() = 0;
};
}
