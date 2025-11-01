//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/strings/string_id.h"
#include "portal/engine/resources/new/loader/loader.h"


namespace portal::renderer::vulkan {
class VulkanContext;
}

namespace portal::ng::resources
{

class TextureLoader final : public ResourceLoader
{
public:
    TextureLoader(ResourceRegistry& registry, renderer::vulkan::VulkanContext& context);

    Resource* load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    /**
     * Creates a "standalone" texture in the registry, a texture without an associated `ResourceSource`
     * This is used to create some default all white, all black, and "missing" textures
     *
     * @param id The texture id
     * @param data The texture data
     * @param extent The texture extent
     */
    void create_standalone_texture(const StringId& id, std::span<uint32_t> data, vk::Extent3D extent);

private:
    renderer::vulkan::VulkanContext& context;
};

} // portal