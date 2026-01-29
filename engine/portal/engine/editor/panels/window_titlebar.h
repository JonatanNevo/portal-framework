//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_descriptor_set_manager.h"

namespace portal
{
// TODO: is this really a panel?
class WindowTitlebar final : public Panel
{
public:
    WindowTitlebar(ResourceRegistry& registry);
    ~WindowTitlebar() override;
    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context) override;
    [[nodiscard]] float get_height() const { return height; }

private:
    float height = 0;
    vk::DescriptorSet logo_descriptor_set;

    // Window button icons
    vk::DescriptorSet minimize_icon;
    vk::DescriptorSet maximize_icon;
    vk::DescriptorSet restore_icon;
    vk::DescriptorSet close_icon;
};
} // portal
