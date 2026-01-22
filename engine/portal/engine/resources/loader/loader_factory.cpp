//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include "font_loader.h"
#include "texture_loader.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/resources/loader/gltf_loader.h"
#include "portal/engine/resources/loader/material_loader.h"
#include "portal/engine/resources/loader/mesh_loader.h"
#include "portal/engine/resources/loader/scene_loader.h"
#include "portal/engine/resources/loader/shader_loader.h"

namespace portal::resources
{
LoaderFactory::LoaderFactory(const Project& project, ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context) : stub_loader(registry), context(context)
{
    loaders[ResourceType::Texture] = std::make_shared<TextureLoader>(registry, context);
    loaders[ResourceType::Shader] = std::make_shared<ShaderLoader>(registry, context);
    loaders[ResourceType::Material] = std::make_shared<MaterialLoader>(project, registry, context);
    loaders[ResourceType::Mesh] = std::make_shared<MeshLoader>(registry, context);
    loaders[ResourceType::Scene] = std::make_shared<SceneLoader>(registry);
    loaders[ResourceType::Composite] = std::make_shared<GltfLoader>(registry, context);
    loaders[ResourceType::Font] = std::make_shared<FontLoader>(registry);
}

ResourceLoader& LoaderFactory::get(const SourceMetadata& meta)
{
    if (!loaders.contains(meta.type))
        return stub_loader;

    return *loaders[meta.type];
}

void LoaderFactory::enrich_metadata(SourceMetadata& meta, const ResourceSource& source)
{
    switch (meta.type)
    {
    case ResourceType::Texture:
        TextureLoader::enrich_metadata(meta, source);
        break;
    case ResourceType::Composite:
        GltfLoader::enrich_metadata(meta, source);
        break;
    case ResourceType::Material:
        MaterialLoader::enrich_metadata(meta, source);
        break;
    case ResourceType::Font:
        FontLoader::enrich_metadata(meta, source);
        break;
    default:
        break;
    }
}
} // portal
