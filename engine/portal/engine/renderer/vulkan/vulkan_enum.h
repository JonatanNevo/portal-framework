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

vk::Format to_format(ImageFormat format);

vk::ShaderStageFlagBits to_shader_stage(ShaderStage stage);

/// Pipeline Enums
vk::PrimitiveTopology to_primitive_topology(PrimitiveTopology topology);
vk::CompareOp to_compare_op(DepthCompareOperator op);
vk::PipelineStageFlagBits to_pipeline_stage(PipelineStage stage);
vk::AccessFlagBits to_access_flag(ResourceAccessFlags flags);

vk::Format to_format(const reflection::Property& prop);


}
