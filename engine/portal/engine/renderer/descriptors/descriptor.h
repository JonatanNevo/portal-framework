//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/core/buffer.h"
#include "portal/engine/renderer/descriptors/descriptor_types.h"

namespace portal::renderer
{

class BufferDescriptor : public RefCounted
{
public:
    explicit BufferDescriptor(const DescriptorResourceType type) : type(type) {};

    virtual void set_data(Buffer data, size_t offset = 0) = 0;
    virtual const Buffer& get_data() const = 0;

    DescriptorResourceType get_type() const { return type; };

protected:
    DescriptorResourceType type;
};

}
