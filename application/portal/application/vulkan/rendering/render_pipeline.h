//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once
#include <memory>
#include <vector>

#include "portal/application/vulkan/common.h"

namespace portal::vulkan
{
class RenderTarget;
class CommandBuffer;
}

namespace portal::vulkan::rendering
{
class Subpass;

/**
 * @brief A RenderPipeline is a sequence of Subpass objects.
 * Subpass holds shaders and can draw the core::sg::Scene.
 * More subpasses can be added to the sequence if required.
 * For example, postprocessing can be implemented with two pipelines which
 * share render targets.
 *
 * GeometrySubpass -> Processes Scene for Shaders, use by itself if shader requires no lighting
 * ForwardSubpass -> Binds lights at the beginning of a GeometrySubpass to create Forward Rendering, should be used with most default shaders
 * LightingSubpass -> Holds a Global Light uniform, Can be combined with GeometrySubpass to create Deferred Rendering
 */
class RenderPipeline final
{
    explicit RenderPipeline(std::vector<std::unique_ptr<rendering::Subpass>>&& subpasses = {});

    RenderPipeline(RenderPipeline&&) noexcept = delete;
    virtual ~RenderPipeline() = default;
    RenderPipeline& operator=(RenderPipeline&&) = default;
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    /**
     * @brief Prepares the subpasses
     */
    void prepare() const;

    /**
     * @return Load store info
     */
    const std::vector<LoadStoreInfo>& get_load_store() const;
    /**
     * @param load_store Load store info to set
     */
    void set_load_store(const std::vector<LoadStoreInfo>& load_store);

    /**
     * @return Clear values
     */
    const std::vector<vk::ClearValue>& get_clear_value() const;

    /**
     * @param clear_values Clear values to set
     */
    void set_clear_value(const std::vector<vk::ClearValue>& clear_values);

    /**
     * @brief Appends a subpass to the pipeline
     * @param subpass Subpass to append
     */
    void add_subpass(std::unique_ptr<rendering::Subpass>&& subpass);

    std::vector<std::unique_ptr<rendering::Subpass>>& get_subpasses();

    /**
     * @brief Record draw commands for each Subpass
     */
    void draw(CommandBuffer& command_buffer, RenderTarget& render_target, vk::SubpassContents contents = vk::SubpassContents::eInline);

    /**
     * @return Subpass currently being recorded, or the first one
     *         if drawing has not started
     */
    std::unique_ptr<rendering::Subpass>& get_active_subpass();

private:
    std::vector<std::unique_ptr<rendering::Subpass>> subpasses;

    /// Default to two load store
    std::vector<LoadStoreInfo> load_store = std::vector<LoadStoreInfo>(2);
    /// Default to two clear values
    std::vector<vk::ClearValue> clear_value = std::vector<vk::ClearValue>(2);

    size_t active_subpass_index{0};
};
} // portal
