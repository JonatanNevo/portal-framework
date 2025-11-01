//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include "texture_loader.h"
#include "portal/engine/renderer/renderer_context.h"

namespace portal::resources
{

LoaderFactory::LoaderFactory(ResourceRegistry& registry, RendererContext& context) : stub_loader(registry), context(context)
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
