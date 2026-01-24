//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "memory_source.h"

#include "portal/core/buffer_stream.h"

namespace portal::resources
{
MemorySource::MemorySource(Buffer&& data) : buffer(data) {}

Buffer MemorySource::load() const
{
    return buffer;
}

Buffer MemorySource::load(const size_t offset, const size_t size) const
{
    return {buffer, offset, size};
}

std::unique_ptr<std::istream> MemorySource::istream() const
{
    return std::make_unique<BufferStreamReader>(buffer);
}

void MemorySource::save(const Buffer data, const size_t offset)
{
    buffer = Buffer::copy(data, offset);
}

std::unique_ptr<std::ostream> MemorySource::ostream()
{
    return std::make_unique<BufferStreamWriter>(buffer);
}
} // portal
