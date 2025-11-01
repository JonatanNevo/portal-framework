//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/resources/loader/loader.h"

namespace portal::renderer
{
namespace vulkan
{
    class VulkanContext;
}

class Shader;
}

namespace portal::resources
{
class ShaderLoader final : public ResourceLoader
{
public:
    ShaderLoader(ResourceRegistry& registry, renderer::vulkan::VulkanContext& context);

    Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) override;

protected:
    Reference<Resource> load_shader(const SourceMetadata& meta, const ResourceSource& source) const;
    Reference<Resource> load_precompiled_shader(const SourceMetadata& meta, const ResourceSource& source) const;

private:
    renderer::vulkan::VulkanContext& context;
};
} // portal
