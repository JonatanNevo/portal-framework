//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan.hpp>
#include "portal/engine/resources/loader/loader.h"

namespace portal::renderer
{
namespace vulkan
{
    class GpuContext;
    class VulkanImage;
}

class Texture;
}

namespace vk::raii
{
class CommandBuffer;
}

namespace portal::resources
{

class TextureLoader final : public ResourceLoader
{
public:
    explicit TextureLoader(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context);

    void initialize() override;
    [[nodiscard]] bool load(StringId id, std::shared_ptr<ResourceSource> source) const override;
    void load_default(Ref<Resource>& resource) const override;

private:
    Ref<renderer::Texture> create_default_texture(const StringId& id, std::span<uint32_t> data, vk::Extent3D extent) const;

    std::shared_ptr<renderer::vulkan::GpuContext> gpu_context;
    WeakRef<renderer::Texture> missing_texture;
};

} // portal
