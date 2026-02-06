//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/resource_reference.h"
#include "portal/engine/resources/resource_registry.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"

namespace portal
{
enum class EditorIcon
{
    Logo,
    // Window Icons
    Minimize,
    Maximize,
    Restore,
    Close,

    // Files Menu Actions
    NewScene,
    NewProject,
    OpenProject,
    OpenRecent,
    Build,
    BuildMenu,
    BuildProject,
    BuildShaders,
    BuildResourceDB,
    Save,
    SaveAll,
    SaveAs,
    Exit,

    // Edit Menu Actions
    Cut,
    Duplicate,
    History,
    Copy,
    Undo,
    Redo,
    Paste,
    Delete
};

class EditorIcons
{
public:
    explicit EditorIcons(ResourceRegistry& registry);
    ~EditorIcons();

    vk::DescriptorSet get_descriptor(EditorIcon name) const;
    ResourceReference<renderer::vulkan::VulkanTexture> get_texture(EditorIcon name);

private:
    void load_image(EditorIcon name, const StringId& texture_id);

private:
    struct image_data
    {
        ResourceReference<renderer::vulkan::VulkanTexture> texture;
        vk::DescriptorSet descriptor;
    };

    ResourceRegistry& registry;
    std::unordered_map<EditorIcon, image_data> images;
};

} // portal