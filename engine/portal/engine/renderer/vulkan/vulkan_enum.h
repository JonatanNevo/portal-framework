//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/image/image_types.h"
#include "portal/engine/renderer/pipeline/pipeline_types.h"
#include "portal/engine/renderer/shaders/shader_types.h"

namespace portal::renderer::vulkan
{
/** @brief Converts ImageFormat to vk::Format */
vk::Format to_format(ImageFormat format);

/** @brief Converts ShaderStage to vk::ShaderStageFlagBits */
vk::ShaderStageFlagBits to_shader_stage(ShaderStage stage);

/** @brief Converts PrimitiveTopology to vk::PrimitiveTopology */
vk::PrimitiveTopology to_primitive_topology(PrimitiveTopology topology);

/** @brief Converts DepthCompareOperator to vk::CompareOp */
vk::CompareOp to_compare_op(DepthCompareOperator op);

/** @brief Converts PipelineStage to vk::PipelineStageFlagBits */
vk::PipelineStageFlagBits to_pipeline_stage(PipelineStage stage);

/** @brief Converts ResourceAccessFlags to vk::AccessFlagBits */
vk::AccessFlagBits to_access_flag(ResourceAccessFlags flags);

/** @brief Converts reflection Property to vk::Format */
vk::Format to_format(const reflection::Property& prop);

/** @brief Converts vk::Format to ImageFormat */
ImageFormat to_format(vk::Format format);

/** @brief Converts TextureFilter to vk::Filter */
vk::Filter to_filter(TextureFilter filter);

/** @brief Converts TextureWrap to vk::SamplerAddressMode */
vk::SamplerAddressMode to_address_mode(TextureWrap warp);

/** @brief Converts SamplerMipmapMode to vk::SamplerMipmapMode */
vk::SamplerMipmapMode to_mipmap_mode(SamplerMipmapMode mode);
}
