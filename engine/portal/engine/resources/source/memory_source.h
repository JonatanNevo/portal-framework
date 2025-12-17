//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{
class MemorySource final : public ResourceSource
{
public:
    explicit MemorySource(Buffer&& data);

    [[nodiscard]] Buffer load() const override;
    [[nodiscard]] Buffer load(size_t offset, size_t size) const override;
    [[nodiscard]] std::unique_ptr<std::istream> stream() const override;

protected:
    Buffer data;
};
} // portal
