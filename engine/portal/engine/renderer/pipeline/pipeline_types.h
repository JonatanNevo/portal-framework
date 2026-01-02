//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once


#include "portal/core/strings/string_id.h"

namespace portal::renderer
{
class ShaderVariant;
}

namespace portal::renderer
{
/**
 * @enum PrimitiveTopology
 * @brief Vertex assembly primitive topology
 *
 * Defines how vertices are assembled into primitives (points, lines, triangles).
 */
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

/**
 * @enum DepthCompareOperator
 * @brief Depth test comparison function
 *
 * Determines when fragments pass the depth test.
 */
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

/**
 * @enum PipelineStage
 * @brief Pipeline execution stages
 *
 * Identifies stages in the graphics/compute/transfer pipeline for synchronization.
 */
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

/**
 * @enum ResourceAccessFlags
 * @brief Memory access types for pipeline barriers
 *
 * Specifies how resources are accessed during pipeline stages for synchronization.
 */
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
}
