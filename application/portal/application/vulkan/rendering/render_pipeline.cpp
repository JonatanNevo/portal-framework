//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "render_pipeline.h"

#include "portal/application/vulkan/debug_utils.h"
#include "portal/application/vulkan/rendering/subpass.h"

namespace portal::vulkan::rendering
{
RenderPipeline::RenderPipeline(std::vector<std::unique_ptr<rendering::Subpass>>&& subpasses): subpasses(std::move(subpasses))
{
    prepare();

    // Default load/store for swapchain
    load_store[0].load_op = vk::AttachmentLoadOp::eClear;
    load_store[0].store_op = vk::AttachmentStoreOp::eStore;

    // Default load/store for depth attachment
    load_store[1].load_op = vk::AttachmentLoadOp::eClear;
    load_store[1].store_op = vk::AttachmentStoreOp::eDontCare;

    // Default clear value
    clear_value[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clear_value[1].setDepthStencil({0.0f, ~0U});
}

void RenderPipeline::prepare() const
{
    for (const auto& subpass : subpasses)
        subpass->prepare();
}

const std::vector<LoadStoreInfo>& RenderPipeline::get_load_store() const
{
    return load_store;
}

void RenderPipeline::set_load_store(const std::vector<LoadStoreInfo>& load_store)
{
    this->load_store = load_store;
}

const std::vector<vk::ClearValue>& RenderPipeline::get_clear_value() const
{
    return clear_value;
}

void RenderPipeline::set_clear_value(const std::vector<vk::ClearValue>& clear_values)
{
    this->clear_value = clear_values;
}

void RenderPipeline::add_subpass(std::unique_ptr<rendering::Subpass>&& subpass)
{
    subpass->prepare();
    subpasses.emplace_back(std::move(subpass));
}

std::vector<std::unique_ptr<rendering::Subpass>>& RenderPipeline::get_subpasses()
{
    return subpasses;
}

void RenderPipeline::draw(CommandBuffer& command_buffer, RenderTarget& render_target, vk::SubpassContents contents)
{
    PORTAL_CORE_ASSERT(!subpasses.empty(), "Render pipeline should contain at least one sub-pass");

    // Pad clear values if they're less than render target attachments
    while (clear_value.size() < render_target.get_attachments().size())
        clear_value.emplace_back(vk::ClearValue({0.0f, 0.0f, 0.0f, 1.0f}));

    for (size_t i = 0; i < subpasses.size(); ++i)
    {
        active_subpass_index = i;
        const auto& subpass = subpasses[i];

        subpass->update_render_target_attachments(render_target);
        if (i == 0)
            command_buffer.begin_render_pass(render_target, load_store, clear_value, subpasses, contents);
        else
            command_buffer.next_subpass();

        if (subpass->get_debug_name().empty())
            subpass->set_debug_name(std::format("Subpass #{}", i));

        ScopedDebugLabel subpass_debug_label(command_buffer, subpass->get_debug_name().c_str());
        subpass->draw(command_buffer);
    }

    active_subpass_index = 0;
}

std::unique_ptr<rendering::Subpass>& RenderPipeline::get_active_subpass()
{
    return subpasses[active_subpass_index];
}
} // portal
