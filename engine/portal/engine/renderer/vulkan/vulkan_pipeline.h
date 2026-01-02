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

/**
 * @class VulkanPipeline
 * @brief Vulkan graphics pipeline
 *
 * Creates vk::Pipeline from PipelineProperties with shader stages, rasterization state,
 * depth testing, and render pass compatibility.
 */
class VulkanPipeline final : public Pipeline
{
public:
    /**
     * @brief Constructs Vulkan pipeline
     * @param prop Pipeline configuration
     * @param context Vulkan context
     */
    explicit VulkanPipeline(const PipelineProperties& prop, const VulkanContext& context);
    ~VulkanPipeline() override;

    /** @brief Gets pipeline properties (mutable) */
    [[nodiscard]] PipelineProperties& get_properties() override;

    /** @brief Gets pipeline properties */
    [[nodiscard]] const PipelineProperties& get_properties() const override;

    /** @brief Gets pipeline shader */
    [[nodiscard]] Reference<ShaderVariant> get_shader() const override;

    /** @brief Checks if line width is dynamic state */
    bool is_dynamic_line_width() const;

    /** @brief Gets Vulkan pipeline handle */
    vk::Pipeline get_vulkan_pipeline();

    /** @brief Gets Vulkan pipeline layout */
    vk::PipelineLayout get_vulkan_pipeline_layout();

private:
    /** @brief Creates Vulkan pipeline and layout */
    void initialize();

private:
    const VulkanContext& context;
    PipelineProperties prop;

    vk::raii::Pipeline pipeline = nullptr;
    vk::raii::PipelineLayout pipeline_layout = nullptr;
};
} // portal
