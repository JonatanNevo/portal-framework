//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reference.h>
#include "portal/engine/strings/string_id.h"

namespace portal::renderer
{
class RenderTarget;
class Shader;

enum class PrimitiveTopology
{
    None,
    Points,
    Lines,
    Triangles,
    LineStrip,
    TriangleStrip,
    TriangleFan
};

enum class DepthCompareOperator
{
    None,
    Never,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    Equal,
    Always,
};


enum class PipelineStage
{
    None,
    TopOfPipe,
    DrawIndirect,
    VertexInput,
    VertexShader,
    TessellationControlShader,
    TessellationEvaluationShader,
    GeometryShader,
    FragmentShader,
    EarlyFragmentTests,
    LateFragmentTests,
    ColorAttachmentOutput,
    ComputeShader,
    Transfer,
    BottomOfPipe,
    Host,
    AllGraphics,
    AllCommands,
    AccelerationStructureBuild,
    RayTracingShader,
    MeshShader
};

enum class ResourceAccessFlags
{
    None,
    IndirectCommandRead,
    IndexRead,
    VertexAttributeRead,
    UniformRead,
    InputAttachmentRead,
    ShaderRead,
    ShaderWrite,
    ColorAttachmentRead,
    ColorAttachmentWrite,
    DepthStencilAttachmentRead,
    DepthStencilAttachmentWrite,
    TransferRead,
    TransferWrite,
    HostRead,
    HostWrite,
    MemoryRead,
    MemoryWrite,
    AccelerationStructureRead,
    AccelerationStructureWrite
};

namespace pipeline
{
    struct Specification
    {
        Ref<Shader> shader;
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

}
