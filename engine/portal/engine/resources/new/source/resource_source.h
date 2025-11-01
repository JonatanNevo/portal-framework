//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"

namespace portal::ng::resources
{

class ResourceSource
{
public:
    virtual ~ResourceSource() = default;
    virtual Buffer load() const = 0;
    virtual Buffer load(size_t offset, size_t size) const = 0;
    virtual std::unique_ptr<std::istream> stream() const = 0;
};

}
