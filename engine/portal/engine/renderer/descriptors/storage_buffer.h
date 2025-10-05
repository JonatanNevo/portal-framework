//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/buffer.h"
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
    StorageBuffer(): BufferDescriptor(DescriptorResourceType::StorageBuffer) {};

    virtual void resize(size_t new_size) = 0;
};

class StorageBufferSet: public BufferDescriptor
{
public:
    StorageBufferSet(): BufferDescriptor(DescriptorResourceType::StorageBufferSet) {};

    virtual Ref<StorageBuffer> get(size_t index) = 0;

    virtual void set(Ref<StorageBuffer> buffer, size_t index) = 0;
};

}
