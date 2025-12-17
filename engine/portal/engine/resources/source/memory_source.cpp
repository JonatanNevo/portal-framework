//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "memory_source.h"

#include "portal/core/buffer_stream.h"

namespace portal::resources
{
MemorySource::MemorySource(Buffer&& data) : data(data) {}

Buffer MemorySource::load() const
{
    return data;
}

Buffer MemorySource::load(const size_t offset, const size_t size) const
{
    return {data, offset, size};
}

std::unique_ptr<std::istream> MemorySource::stream() const
{
    return std::make_unique<BufferStreamReader>(data);
}
} // portal
