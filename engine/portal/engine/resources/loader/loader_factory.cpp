//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include "texture_loader.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/loader/gltf_loader.h"
#include "portal/engine/resources/loader/shader_loader.h"

namespace portal::resources
{

LoaderFactory::LoaderFactory(ResourceRegistry& registry, const RendererContext& context) : stub_loader(registry), context(context)
{
    loaders[ResourceType::Texture] = std::make_shared<TextureLoader>(registry, context);
    loaders[ResourceType::Shader] = std::make_shared<ShaderLoader>(registry, context);
    const auto gltf_loader = std::make_shared<GltfLoader>(registry, context);
    loaders[ResourceType::Scene] = gltf_loader;
    loaders[ResourceType::Composite] = gltf_loader;
}

ResourceLoader& LoaderFactory::get(const SourceMetadata& meta)
{
    if (!loaders.contains(meta.type))
        return stub_loader;

    return *loaders[meta.type];
}
} // portal
