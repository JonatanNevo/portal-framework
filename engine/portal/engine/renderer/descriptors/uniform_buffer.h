//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/descriptors/descriptor.h"

namespace portal::renderer
{

class UniformBuffer: public BufferDescriptor
{
public:
    UniformBuffer(const StringId& id): BufferDescriptor(id, DescriptorResourceType::UniformBuffer) {};
};

class UniformBufferSet: public BufferDescriptor
{
public:
    UniformBufferSet(const StringId& id): BufferDescriptor(id, DescriptorResourceType::UniformBufferSet) {};

    virtual Reference<UniformBuffer> get(size_t index) = 0;

    virtual void set(const Reference<UniformBuffer>& buffer, size_t index) = 0;
};

}
