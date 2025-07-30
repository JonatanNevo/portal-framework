//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "loader_factory.h"

#include "portal/engine/resources/loader/image_loader.h"
#include "portal/engine/resources/utils.h"

namespace portal::resources
{
static auto logger = spdlog::get("Resources");

void LoaderFactory::initialize(const std::shared_ptr<GpuContext>& context)
{
    gpu_context = context;
    stub_loader = std::make_shared<StubLoader>();
}

std::shared_ptr<ResourceLoader> LoaderFactory::get(const SourceMetadata metadata)
{
    switch (metadata.resource_type)
    {
    case ResourceType::Material:
        break;
    case ResourceType::Texture:
        return get_texture_loader(metadata);
    case ResourceType::Shader:
        break;
    case ResourceType::Mesh:
        break;
    case ResourceType::Composite:
        break;
    case ResourceType::Unknown:
        break;
    }
    return stub_loader;
}

std::shared_ptr<ResourceLoader> LoaderFactory::get_texture_loader(const SourceMetadata metadata)
{
    switch (metadata.format)
    {
    case SourceFormat::Image:
        return std::make_shared<ImageLoader>(gpu_context);
    case SourceFormat::Texture:
        break;
    case SourceFormat::Preprocessed:
        break;
    default:
        break;
    }
    return stub_loader;
}

}
