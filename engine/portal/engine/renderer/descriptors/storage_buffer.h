//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/descriptors/descriptor.h"
#include "portal/engine/strings/string_id.h"

namespace portal::renderer
{

struct StorageBufferSpecification
{
    size_t size;
    bool gpu_only = true;
    StringId debug_name;
};

class StorageBuffer: public BufferDescriptor
{
public:
    explicit StorageBuffer(const StringId& id): BufferDescriptor(id, DescriptorResourceType::StorageBuffer) {};

    virtual void resize(size_t new_size) = 0;
};

class StorageBufferSet: public BufferDescriptor
{
public:
    explicit StorageBufferSet(const StringId& id): BufferDescriptor(id, DescriptorResourceType::StorageBufferSet) {};

    virtual Reference<StorageBuffer> get(size_t index) = 0;

    virtual void set(const Reference<StorageBuffer>& buffer, size_t index) = 0;
};

}
