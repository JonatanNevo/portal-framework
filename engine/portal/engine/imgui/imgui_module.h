//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{

class ImGuiModule: public TaggedModule<Tag<tags::FrameLifecycle, tags::Gui>, Renderer>
{
public:
    ImGuiModule(ModuleStack& stack, const Window& window);
    ~ImGuiModule() override;

    void begin_frame(renderer::FrameContext& frame) override;
    void end_frame(renderer::FrameContext& frame) override;

    void gui_update(renderer::FrameContext& frame) override;

private:
    vk::raii::DescriptorPool imgui_pool = nullptr;
};
} // portal
