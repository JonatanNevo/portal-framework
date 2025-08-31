//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vertex_buffer.h"

namespace portal::renderer
{

VertexBufferLayout::VertexBufferLayout(std::initializer_list<ShaderBufferElement> elements): elements(elements)
{}

size_t VertexBufferLayout::get_stride() const
{
    return stride;
}

const std::vector<ShaderBufferElement>& VertexBufferLayout::get_elements() const
{
    return elements;
}

size_t VertexBufferLayout::get_element_count() const
{
    return elements.size();
}

void VertexBufferLayout::calculate_offsets_and_stride()
{
    size_t offset = 0;
    stride = 0;

    for (auto& element : elements)
    {
        element.offset = offset;
        offset += element.size;
        stride += element.size;
    }
}
} // portal
