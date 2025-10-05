//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/engine/renderer/pipeline/pipeline_types.h"

namespace portal::renderer
{
class RenderTarget;

namespace pipeline
{
    struct Specification
    {
        Ref<ShaderVariant> shader;
        Ref<RenderTarget> render_target;

        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        DepthCompareOperator depth_compare_operator = DepthCompareOperator::GreaterOrEqual;

        bool backface_culling = true;
        bool depth_test = true;
        bool depth_write = true;
        bool wireframe = false;

        float line_width = 1.f;

        StringId debug_name = INVALID_STRING_ID;
    };

    struct Statistics
    {
        size_t vertices = 0;
        size_t primitives = 0;
        size_t vertex_shader_invocations = 0;
        size_t clipping_invocations = 0;
        size_t clipping_primitives = 0;
        size_t fragment_shader_invocations = 0;
        size_t compute_shader_invocations = 0;
    };
}

class Pipeline : public RefCounted
{
public:
    [[nodiscard]] virtual pipeline::Specification& get_spec() = 0;
    [[nodiscard]] virtual const pipeline::Specification& get_spec() const = 0;

    virtual void initialize() = 0;

    [[nodiscard]] virtual Ref<ShaderVariant> get_shader() const = 0;
};
} // portal