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
    explicit VulkanPipeline(const PipelineProperties& prop, const VulkanContext& context);
    ~VulkanPipeline() override;

    [[nodiscard]] PipelineProperties& get_properties() override;
    [[nodiscard]] const PipelineProperties& get_properties() const override;

    [[nodiscard]] Reference<ShaderVariant> get_shader() const override;

    bool is_dynamic_line_width() const;

    vk::Pipeline get_vulkan_pipeline();
    vk::PipelineLayout get_vulkan_pipeline_layout();

private:
    void initialize();

private:
    const VulkanContext& context;
    PipelineProperties prop;

    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout pipeline_layout = nullptr;
};
} // portal
