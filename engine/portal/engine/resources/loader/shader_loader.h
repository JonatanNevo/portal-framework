//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/resources/loader/loader.h"

namespace portal
{
class RendererContext;
}


namespace portal::resources
{
class ShaderLoader final : public ResourceLoader
{
public:
    ShaderLoader(ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context);

     ResourceData load(const SourceMetadata& meta, Reference<ResourceSource> source) override;
    void save(const ResourceData& resource_data) override;

protected:
    [[nodiscard]] Reference<Resource> load_shader(const SourceMetadata& meta, const ResourceSource& source) const;
    [[nodiscard]] Reference<Resource> load_precompiled_shader(const SourceMetadata& meta, const ResourceSource& source) const;

private:
    const renderer::vulkan::VulkanContext& context;
};
} // portal
