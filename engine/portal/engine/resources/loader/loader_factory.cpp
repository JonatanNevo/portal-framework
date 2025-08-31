//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include <ranges>

#include "portal/engine/resources/loader/texture_loader.h"
#include "portal/engine/resources/utils.h"
#include "portal/engine/resources/loader/gltf_loader.h"
#include "portal/engine/resources/loader/shader_loader.h"

namespace portal::resources
{
static auto logger = spdlog::get("Resources");

std::unordered_map<StringId, std::shared_ptr<ResourceLoader>> loaders;

void LoaderFactory::initialize(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context)
{
    gpu_context = context;
    stub_loader = std::make_shared<StubLoader>(registry);

    loaders[STRING_ID("Texture")] = std::make_shared<TextureLoader>(registry, gpu_context);
    loaders[STRING_ID("Shader")] = std::make_shared<ShaderLoader>(registry, gpu_context);
    loaders[STRING_ID("glFT")] = std::make_shared<GltfLoader>(registry, gpu_context);

    for (const auto& loader : loaders | std::views::values)
        loader->initialize();
}

void LoaderFactory::shutdown()
{
    gpu_context = nullptr;
    loaders.clear();
}

std::shared_ptr<ResourceLoader> LoaderFactory::get(const ResourceType type)
{
    switch (type)
    {
    case ResourceType::Material:
        break;
    case ResourceType::Texture:
        return loaders[STRING_ID("Texture")];
    case ResourceType::Shader:
        return loaders[STRING_ID("Shader")];
    case ResourceType::Mesh:
        break;
    case ResourceType::Composite:
        return loaders[STRING_ID("glFT")];
    case ResourceType::Pipeline:
        break;
    case ResourceType::Scene:
        return loaders[STRING_ID("glFT")];
    case ResourceType::Unknown:
        break;
    }
    return stub_loader;
}

}
