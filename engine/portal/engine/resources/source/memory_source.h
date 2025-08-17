//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "resource_source.h"

namespace portal::resources
{

class MemorySource final : public ResourceSource
{
public:
    MemorySource(Buffer&& data, const SourceMetadata& metadata);
    [[nodiscard]] SourceMetadata get_meta() const override;
    Buffer load() override;
    Buffer load(size_t offset, size_t size) override;
    std::unique_ptr<std::istream> stream() override;

protected:
    SourceMetadata metadata;
    Buffer data;
};

} // portal
