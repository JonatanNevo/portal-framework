//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace portal
{

enum class MaterialPass: uint8_t
{
    MainColor,
    Transparent,
    Other
};

struct MaterialPipeline
{
    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout layout = nullptr;
};

struct MaterialInstance
{
    MaterialPipeline* pipeline = nullptr;
    vk::raii::DescriptorSet material_set = nullptr;
    MaterialPass pass_type = MaterialPass::Other;
};


} // portal
