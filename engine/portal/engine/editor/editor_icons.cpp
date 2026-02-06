//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_icons.h"

#include "portal/third_party/imgui/backends/imgui_impl_vulkan.h"

namespace portal
{

EditorIcons::EditorIcons(ResourceRegistry& registry) : registry(registry)
{
    // Load window button icons
    load_image(EditorIcon::Logo, STRING_ID("engine/portal_icon_64x64"));

    // Window Icons
    load_image(EditorIcon::Minimize, STRING_ID("engine/editor/icons/window/minimize"));
    load_image(EditorIcon::Maximize, STRING_ID("engine/editor/icons/window/maximize"));
    load_image(EditorIcon::Restore, STRING_ID("engine/editor/icons/window/restore"));
    load_image(EditorIcon::Close, STRING_ID("engine/editor/icons/window/close"));

    // Files Menu Bar
    load_image(EditorIcon::BuildMenu, STRING_ID("engine/editor/icons/generic/blocks"));
    load_image(EditorIcon::BuildShaders, STRING_ID("engine/editor/icons/generic/boxes"));
    load_image(EditorIcon::NewScene, STRING_ID("engine/editor/icons/generic/file-plus-corner"));
    load_image(EditorIcon::BuildProject, STRING_ID("engine/editor/icons/generic/folder-cog"));
    load_image(EditorIcon::OpenProject, STRING_ID("engine/editor/icons/generic/folder-open"));
    load_image(EditorIcon::NewProject, STRING_ID("engine/editor/icons/generic/folder-plus"));
    load_image(EditorIcon::OpenRecent, STRING_ID("engine/editor/icons/generic/folder-clock"));
    load_image(EditorIcon::BuildResourceDB, STRING_ID("engine/editor/icons/generic/folders"));
    load_image(EditorIcon::Build, STRING_ID("engine/editor/icons/generic/hammer"));
    load_image(EditorIcon::SaveAs, STRING_ID("engine/editor/icons/generic/import"));
    load_image(EditorIcon::Exit, STRING_ID("engine/editor/icons/generic/log-out"));
    load_image(EditorIcon::Save, STRING_ID("engine/editor/icons/generic/save"));
    load_image(EditorIcon::SaveAll, STRING_ID("engine/editor/icons/generic/save-all"));

    // Edit Menu Bar
    load_image(EditorIcon::Cut, STRING_ID("engine/editor/icons/generic/scissors"));
    load_image(EditorIcon::Duplicate, STRING_ID("engine/editor/icons/generic/duplicate"));
    load_image(EditorIcon::History, STRING_ID("engine/editor/icons/generic/square-stack"));
    load_image(EditorIcon::Copy, STRING_ID("engine/editor/icons/generic/copy"));
    load_image(EditorIcon::Undo, STRING_ID("engine/editor/icons/generic/undo"));
    load_image(EditorIcon::Redo, STRING_ID("engine/editor/icons/generic/redo"));
    load_image(EditorIcon::Paste, STRING_ID("engine/editor/icons/generic/clipboard"));
    load_image(EditorIcon::Delete, STRING_ID("engine/editor/icons/generic/trash"));
}

EditorIcons::~EditorIcons()
{
    for (auto& [texture, descriptor] : images | std::views::values)
        ImGui_ImplVulkan_RemoveTexture(descriptor);
}

void EditorIcons::load_image(const EditorIcon name, const StringId& texture_id)
{
    auto texture = registry.immediate_load<renderer::vulkan::VulkanTexture>(texture_id);
    const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(texture->get_image());
    const auto& img_info = vulkan_image->get_image_info();
    images[name] = {
        texture,
        ImGui_ImplVulkan_AddTexture(
            img_info.sampler->get_vk_sampler(),
            img_info.view->get_vk_image_view(),
            static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
        )
    };
}

vk::DescriptorSet EditorIcons::get_descriptor(const EditorIcon name) const
{
    return images.at(name).descriptor;
}

ResourceReference<renderer::vulkan::VulkanTexture> EditorIcons::get_texture(const EditorIcon name)
{
    return images.at(name).texture;
}
} // portal
