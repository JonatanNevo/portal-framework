//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <stb_image.h>

#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/resources/loader/loader.h"
#include "portal/engine/resources/resources/texture.h"


namespace vk::raii
{
class CommandBuffer;
}

namespace portal::resources
{
class GpuContext;

class TextureLoader final : public ResourceLoader
{
public:
    explicit TextureLoader(ResourceRegistry* registry, const std::shared_ptr<GpuContext>& context);

    void initialize() override;
    [[nodiscard]] bool load(std::shared_ptr<ResourceSource> source) const override;
    void load_default(Ref<Resource>& resource) const override;

private:
    std::shared_ptr<vulkan::AllocatedImage> build_image_from_memory(const StringId& id, void* data, vk::Extent3D extent) const;
    Ref<Texture> create_default_texture(const StringId& id, std::span<uint32_t> data, vk::Extent3D extent) const;

    std::shared_ptr<GpuContext> gpu_context;
    WeakRef<Texture> missing_texture;
};

} // portal
