//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/reference.h"
#include "portal/engine/renderer/shaders/shader_types.h"

namespace portal::renderer
{

class VertexBufferLayout
{
public:
    VertexBufferLayout() = default;
    VertexBufferLayout(std::initializer_list<ShaderBufferElement> elements);

    [[nodiscard]] size_t get_stride() const;
    [[nodiscard]] const std::vector<ShaderBufferElement>& get_elements() const;
    [[nodiscard]] size_t get_element_count() const;

private:
    void calculate_offsets_and_stride();

private:
    std::vector<ShaderBufferElement> elements;
    size_t stride = 0;
};

class VertexBuffer: public RefCounted
{
public:
    enum class Usage
    {
        None,
        Static,
        Dynamic
    };

public:
    virtual void set_data(Buffer buffer, size_t offset) = 0;
    virtual void bind() const = 0;

    virtual size_t get_size() const = 0;
    virtual Usage get_usage() const = 0;
};

}
