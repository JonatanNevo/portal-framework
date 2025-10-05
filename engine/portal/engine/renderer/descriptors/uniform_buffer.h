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
    UniformBuffer(): BufferDescriptor(DescriptorResourceType::UniformBuffer) {};
};

class UniformBufferSet: public BufferDescriptor
{
public:
    UniformBufferSet(): BufferDescriptor(DescriptorResourceType::UniformBufferSet) {};

    virtual Ref<UniformBuffer> get(size_t index) = 0;

    virtual void set(Ref<UniformBuffer> buffer, size_t index) = 0;
};

}
