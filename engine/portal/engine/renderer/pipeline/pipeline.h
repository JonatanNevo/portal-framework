//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/reference.h"
#include "portal/engine/renderer/pipeline/pipeline_types.h"
#include "portal/engine/renderer/render_target/render_target.h"

namespace portal::renderer
{
struct PipelineProperties
{
    Reference<ShaderVariant> shader;
    AttachmentProperties attachments;

    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    DepthCompareOperator depth_compare_operator = DepthCompareOperator::GreaterOrEqual;

    bool backface_culling = true;
    bool depth_test = true;
    bool depth_write = true;
    bool wireframe = false;

    float line_width = 1.f;

    StringId debug_name = INVALID_STRING_ID;
};

struct PipelineStatistics
{
    size_t vertices = 0;
    size_t primitives = 0;
    size_t vertex_shader_invocations = 0;
    size_t clipping_invocations = 0;
    size_t clipping_primitives = 0;
    size_t fragment_shader_invocations = 0;
    size_t compute_shader_invocations = 0;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    [[nodiscard]] virtual PipelineProperties& get_properties() = 0;
    [[nodiscard]] virtual const PipelineProperties& get_properties() const = 0;

    [[nodiscard]] virtual Reference<ShaderVariant> get_shader() const = 0;
};
} // portal
