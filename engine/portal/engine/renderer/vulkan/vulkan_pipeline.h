//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "portal/engine/renderer/pipeline/pipeline.h"

namespace portal::renderer::vulkan
{
class VulkanContext;

class VulkanPipeline final : public Pipeline
{
public:
    explicit VulkanPipeline(const pipeline::Specification& spec, const VulkanContext& context);
    ~VulkanPipeline() override;

    [[nodiscard]] pipeline::Specification& get_spec() override;
    [[nodiscard]] const pipeline::Specification& get_spec() const override;

    [[nodiscard]] Reference<ShaderVariant> get_shader() const override;

    bool is_dynamic_line_width() const;

    vk::Pipeline get_vulkan_pipeline();
    vk::PipelineLayout get_vulkan_pipeline_layout();

private:
    void initialize();

private:
    const VulkanContext& context;
    pipeline::Specification spec;

    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout pipeline_layout = nullptr;
};

} // portal