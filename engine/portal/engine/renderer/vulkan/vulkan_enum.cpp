//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_enum.h"

namespace portal::renderer::vulkan
{

vk::Format to_format(ImageFormat format)
{
#define CASE(FROM, TO)                       \
case portal::renderer::ImageFormat::FROM:    \
return vk::Format::TO

    switch (format)
    {
    CASE(None, eUndefined);
    CASE(R8_UNorm, eR8Unorm);
    CASE(R8_UInt, eR8Uint);
    CASE(R16_UInt, eR16Uint);
    CASE(R32_UInt, eR32Uint);
    CASE(R16_Float, eR16Sfloat);
    CASE(R32_Float, eR32Sfloat);
    CASE(RG8_UNorm, eR8G8Unorm);
    CASE(RG8_UInt, eR8G8Uint);
    CASE(RG16_UInt, eR16G16Uint);
    CASE(RG32_UInt, eR32G32Uint);
    CASE(RG16_Float, eR16G16Sfloat);
    CASE(RG32_Float, eR32G32Sfloat);
    CASE(RGB8_UNorm, eR8G8B8Unorm);
    CASE(RGB8_UInt, eR8G8B8Uint);
    CASE(RGB16_UInt, eR16G16B16Uint);
    CASE(RGB32_UInt, eR32G32B32Uint);
    CASE(RGB16_Float, eR16G16B16Sfloat);
    CASE(RGB32_Float, eR32G32B32Sfloat);
    CASE(RGBA8_UNorm, eR8G8B8A8Unorm);
    CASE(RGBA8_UInt, eR8G8B8A8Uint);
    CASE(RGBA16_UInt, eR16G16B16A16Uint);
    CASE(RGBA32_UInt, eR32G32B32A32Uint);
    CASE(RGBA16_Float, eR16G16B16A16Sfloat);
    CASE(RGBA32_Float, eR32G32B32A32Sfloat);
    CASE(SRGB, eR8G8B8Srgb);
    CASE(SRGBA, eR8G8B8A8Srgb);
    CASE(Depth_32Float_Stencil_8UInt, eD32SfloatS8Uint);
    CASE(Depth_32Float, eD32Sfloat);
    CASE(Depth_24UNorm_Stencil_8UInt, eD24UnormS8Uint);
    CASE(Depth_16UNorm_Stencil_8UInt, eD16UnormS8Uint);
    CASE(Depth_16UNorm, eD16UnormS8Uint);
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unknown image format stage");
    return vk::Format::eUndefined;

#undef CASE
}

vk::ShaderStageFlagBits to_shader_stage(const ShaderStage stage)
{
#define CASE(FROM, TO)                       \
case portal::renderer::ShaderStage::FROM:    \
return vk::ShaderStageFlagBits::TO

    switch (stage)
    {
    CASE(All, eAll);
    CASE(Vertex, eVertex);
    CASE(Fragment, eFragment);
    CASE(Geometry, eGeometry);
    CASE(Compute, eCompute);
    CASE(RayGeneration, eRaygenKHR);
    CASE(Intersection, eIntersectionKHR);
    CASE(AnyHit, eAnyHitKHR);
    CASE(ClosestHit, eClosestHitKHR);
    CASE(Miss, eMissKHR);
    CASE(Callable, eCallableKHR);
    CASE(Mesh, eMeshEXT);
    }

    PORTAL_ASSERT(false, "Unknown shader stage");
    return vk::ShaderStageFlagBits::eAll;
#undef CASE
}

vk::PrimitiveTopology to_primitive_topology(const PrimitiveTopology topology)
{
#define CASE(FROM, TO)                          \
case portal::renderer::PrimitiveTopology::FROM: \
return vk::PrimitiveTopology::TO

    switch (topology)
    {
    CASE(Points, ePointList);
    CASE(Lines, eLineList);
    CASE(Triangles, eTriangleList);
    CASE(LineStrip, eLineStrip);
    CASE(TriangleStrip, eTriangleStrip);
    CASE(TriangleFan, eTriangleFan);
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unknown topology");
    return static_cast<vk::PrimitiveTopology>(VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);

#undef CASE
}

vk::CompareOp to_compare_op(const DepthCompareOperator op)
{
#define CASE(FROM, TO)                             \
case portal::renderer::DepthCompareOperator::FROM: \
return vk::CompareOp::TO

    switch (op)
    {
    CASE(Never, eNever);
    CASE(NotEqual, eNotEqual);
    CASE(Less, eLess);
    CASE(Equal, eEqual);
    CASE(LessOrEqual, eLessOrEqual);
    CASE(Greater, eGreater);
    CASE(GreaterOrEqual, eGreaterOrEqual);
    CASE(Always, eAlways);
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unknown compare operator");
    return static_cast<vk::CompareOp>(VK_COMPARE_OP_MAX_ENUM);

#undef CASE
}

vk::PipelineStageFlagBits to_pipeline_stage(const PipelineStage stage)
{
#define CASE(FROM, TO)                      \
case portal::renderer::PipelineStage::FROM: \
return vk::PipelineStageFlagBits::TO

    switch (stage)
    {
    CASE(None, eNone);
    CASE(TopOfPipe, eTopOfPipe);
    CASE(DrawIndirect, eDrawIndirect);
    CASE(VertexInput, eVertexInput);
    CASE(VertexShader, eVertexShader);
    CASE(TessellationControlShader, eTessellationControlShader);
    CASE(TessellationEvaluationShader, eTessellationEvaluationShader);
    CASE(GeometryShader, eGeometryShader);
    CASE(FragmentShader, eFragmentShader);
    CASE(EarlyFragmentTests, eEarlyFragmentTests);
    CASE(LateFragmentTests, eLateFragmentTests);
    CASE(ColorAttachmentOutput, eColorAttachmentOutput);
    CASE(ComputeShader, eComputeShader);
    CASE(Transfer, eTransfer);
    CASE(BottomOfPipe, eBottomOfPipe);
    CASE(Host, eHost);
    CASE(AllGraphics, eAllGraphics);
    CASE(AllCommands, eAllCommands);
    CASE(AccelerationStructureBuild, eAccelerationStructureBuildKHR);
    CASE(RayTracingShader, eRayTracingShaderKHR);
    CASE(MeshShader, eMeshShaderEXT);
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unknown pipeline stage");
    return vk::PipelineStageFlagBits::eNone;

#undef CASE
}

vk::AccessFlagBits to_access_flag(const ResourceAccessFlags flags)
{
#define CASE(FROM, TO)                            \
case portal::renderer::ResourceAccessFlags::FROM: \
return vk::AccessFlagBits::TO

    switch (flags)
    {
    CASE(None, eNone);
    CASE(IndirectCommandRead, eIndirectCommandRead);
    CASE(IndexRead, eIndexRead);
    CASE(VertexAttributeRead, eVertexAttributeRead);
    CASE(UniformRead, eUniformRead);
    CASE(InputAttachmentRead, eInputAttachmentRead);
    CASE(ShaderRead, eShaderRead);
    CASE(ShaderWrite, eShaderWrite);
    CASE(ColorAttachmentRead, eColorAttachmentRead);
    CASE(ColorAttachmentWrite, eColorAttachmentWrite);
    CASE(DepthStencilAttachmentRead, eDepthStencilAttachmentRead);
    CASE(DepthStencilAttachmentWrite, eDepthStencilAttachmentWrite);
    CASE(TransferRead, eTransferRead);
    CASE(TransferWrite, eTransferWrite);
    CASE(HostRead, eHostRead);
    CASE(HostWrite, eHostWrite);
    CASE(MemoryRead, eMemoryRead);
    CASE(MemoryWrite, eMemoryWrite);
    CASE(AccelerationStructureRead, eAccelerationStructureReadKHR);
    CASE(AccelerationStructureWrite, eAccelerationStructureWriteKHR);
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unknown access flag");
    return vk::AccessFlagBits::eNone;
#undef CASE
}

vk::Format to_format(const reflection::Property& prop)
{
    switch (prop.container_type)
    {
    case reflection::PropertyContainerType::scalar:
    {
        switch (prop.type)
        {
        case reflection::PropertyType::boolean:
        case reflection::PropertyType::character:
        case reflection::PropertyType::integer8:
            return vk::Format::eR8Sint;
        case reflection::PropertyType::integer16:
            return vk::Format::eR16Sint;
        case reflection::PropertyType::integer32:
            return vk::Format::eR32Sint;
        case reflection::PropertyType::integer64:
            return vk::Format::eR64Sint;
        case reflection::PropertyType::floating32:
            return vk::Format::eR32Sfloat;
        case reflection::PropertyType::floating64:
            return vk::Format::eR64Sfloat;
        default:
            PORTAL_ASSERT(false, "Unsupported container type");
            return vk::Format::eUndefined;
        }
    }
    case reflection::PropertyContainerType::vector:
    {
        switch (prop.elements_number)
        {
        case 2:
        {
            switch (prop.type)
            {
            case reflection::PropertyType::boolean:
            case reflection::PropertyType::character:
            case reflection::PropertyType::integer8:
                return vk::Format::eR8G8Sint;
            case reflection::PropertyType::integer16:
                return vk::Format::eR16G16Sint;
            case reflection::PropertyType::integer32:
                return vk::Format::eR32G32Sint;
            case reflection::PropertyType::integer64:
                return vk::Format::eR64G64Sint;
            case reflection::PropertyType::floating32:
                return vk::Format::eR32G32Sfloat;
            case reflection::PropertyType::floating64:
                return vk::Format::eR64G64Sfloat;
            default:
                PORTAL_ASSERT(false, "Unsupported container type");
                return vk::Format::eUndefined;
            }
        }
        case 3:
        {
            switch (prop.type)
            {
            case reflection::PropertyType::boolean:
            case reflection::PropertyType::character:
            case reflection::PropertyType::integer8:
                return vk::Format::eR8G8B8Sint;
            case reflection::PropertyType::integer16:
                return vk::Format::eR16G16B16Sint;
            case reflection::PropertyType::integer32:
                return vk::Format::eR32G32B32Sint;
            case reflection::PropertyType::integer64:
                return vk::Format::eR64G64B64Sint;
            case reflection::PropertyType::floating32:
                return vk::Format::eR32G32B32Sfloat;
            case reflection::PropertyType::floating64:
                return vk::Format::eR64G64B64Sfloat;
            default:
                PORTAL_ASSERT(false, "Unsupported container type");
                return vk::Format::eUndefined;
            }
        }
        case 4:
        {
            switch (prop.type)
            {
            case reflection::PropertyType::boolean:
            case reflection::PropertyType::character:
            case reflection::PropertyType::integer8:
                return vk::Format::eR8G8B8A8Sint;
            case reflection::PropertyType::integer16:
                return vk::Format::eR16G16B16A16Sint;
            case reflection::PropertyType::integer32:
                return vk::Format::eR32G32B32A32Sint;
            case reflection::PropertyType::integer64:
                return vk::Format::eR64G64B64A64Sint;
            case reflection::PropertyType::floating32:
                return vk::Format::eR32G32B32A32Sfloat;
            case reflection::PropertyType::floating64:
                return vk::Format::eR64G64B64A64Sfloat;
            default:
                PORTAL_ASSERT(false, "Unsupported container type");
                return vk::Format::eUndefined;
            }
        }
        default:
            PORTAL_ASSERT(false, "Unsupported container type");
            return vk::Format::eUndefined;
        }
    }
    default:
        break;
    }

    PORTAL_ASSERT(false, "Unsupported container type");
    return vk::Format::eUndefined;
}
}
