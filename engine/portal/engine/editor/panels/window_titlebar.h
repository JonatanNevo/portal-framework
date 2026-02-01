//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"

#include <vulkan/vulkan.hpp>
#include "portal/engine/imgui/theme/editor_theme.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"

namespace portal
{

class ImGuiImages
{
public:
    ImGuiImages(ResourceRegistry& registry);
    ~ImGuiImages();

    void load_image(const StringId& name, const StringId& texture_id);

    vk::DescriptorSet get_descriptor(const StringId& name) const;
    ResourceReference<renderer::vulkan::VulkanTexture> get_texture(const StringId& name);

private:
    struct image_data
    {
        ResourceReference<renderer::vulkan::VulkanTexture> texture;
        vk::DescriptorSet descriptor;
    };

    ResourceRegistry& registry;
    std::unordered_map<StringId, image_data> images;
};

// TODO: is this really a panel?
class WindowTitlebar final : public Panel
{
public:
    WindowTitlebar(ResourceRegistry& registry, EditorContext& context);
    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context) override;
    [[nodiscard]] float get_height() const { return height; }

private:
    void draw_menubar(EditorContext& editor_context);

private:
    float height = 0;
    bool titlebar_hovered = false;

    ImGuiImages icons;

    ImVec4 active_color;
    ImVec4 target_color;
    ImVec4 previous_color;
    bool animate_titlebar_color = true;
};
} // portal
