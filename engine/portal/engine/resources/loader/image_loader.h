//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/resources/loader/loader.h"


namespace vk::raii
{
class CommandBuffer;
}

namespace portal::resources
{
class GpuContext;

class ImageLoader final : public ResourceLoader
{
public:
    explicit ImageLoader(const std::shared_ptr<GpuContext>& runner);

    void init(std::shared_ptr<ResourceSource> resource_source) override;
    bool load(const Ref<Resource>& resource) override;
    void load_default(const Ref<Resource>& resource) const override;
    [[nodiscard]] std::vector<ResourceSignature> get_signature() const override;

private:
    [[nodiscard]] vulkan::AllocatedImage create_default_texture() const;

private:
    std::shared_ptr<GpuContext> gpu_context;
    std::shared_ptr<ResourceSource> source;
    Buffer default_texture_data;
};

} // portal
