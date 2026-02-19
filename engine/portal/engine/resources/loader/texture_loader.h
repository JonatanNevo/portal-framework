//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/core/strings/string_id.h"
#include "loader.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"


namespace portal
{
class RendererContext;
}

namespace portal::resources
{
class TextureLoader final : public ResourceLoader
{
public:
    TextureLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context);

    ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) override;
    static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);
    void save(ResourceData& resource_data) override;

protected:
    /**
     * Creates a "standalone" texture in the registry, a texture without an associated `ResourceSource`
     * This is used to create some default all white, all black, and "missing" textures
     *
     * @param id The texture id
     * @param data The texture data
     * @param extent The texture extent
     * @param type The type of the texture (cubed or not)
     */
    void create_standalone_texture(
        const StringId& id,
        std::span<uint32_t> data,
        vk::Extent3D extent,
        renderer::TextureType type = renderer::TextureType::Texture
    ) const;

private:
    const renderer::vulkan::VulkanContext& context;
};
} // portal
