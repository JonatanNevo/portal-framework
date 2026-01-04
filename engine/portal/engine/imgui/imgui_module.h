//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{
class ImGuiModule : public TaggedModule<Tag<ModuleTags::FrameLifecycle, ModuleTags::GuiUpdate>, Renderer>
{
public:
    ImGuiModule(ModuleStack& stack, const Window& window, const renderer::vulkan::VulkanSwapchain& swapchain);
    ~ImGuiModule() override;

    void begin_frame(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;

    void gui_update(FrameContext& frame) override;

private:
    vk::raii::DescriptorPool imgui_pool = nullptr;
};
} // portal
