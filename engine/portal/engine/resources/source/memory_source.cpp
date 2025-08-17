//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "memory_source.h"

#include "portal/core/buffer_stream.h"

namespace portal
{

resources::MemorySource::MemorySource(Buffer&& data, const SourceMetadata& metadata) : metadata(metadata), data(std::move(data))
{}

resources::SourceMetadata resources::MemorySource::get_meta() const
{
    return metadata;
}

Buffer resources::MemorySource::load()
{
    return data;
}

Buffer resources::MemorySource::load(const size_t offset, const size_t size)
{
    return Buffer(data, offset, size);
}

std::unique_ptr<std::istream> resources::MemorySource::stream()
{
    return std::make_unique<BufferStreamReader>(data);
}
} // portal
