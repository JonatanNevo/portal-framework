//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include "portal/engine/resources/new/loader/texture_loader.h"

namespace portal::ng::resources
{

LoaderFactory::LoaderFactory(ResourceRegistry& registry, renderer::vulkan::VulkanContext& context) : stub_loader(registry), context(context)
{
    loaders[ResourceType::Texture] = std::make_unique<TextureLoader>(registry, context);
}

ResourceLoader& LoaderFactory::get(const SourceMetadata& meta)
{
    if (!loaders.contains(meta.type))
        return stub_loader;

    return *loaders[meta.type];
}
} // portal
