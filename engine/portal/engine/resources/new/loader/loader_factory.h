//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>

#include "llvm/ADT/DenseMap.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/resources/new/database/resource_database.h"
#include "portal/engine/resources/new/loader/loader.h"

namespace portal::ng {
class ResourceRegistry;
}

namespace portal::renderer::vulkan {
class VulkanContext;
}

namespace portal::ng::resources
{

class StubLoader final : public ResourceLoader
{
public:
    explicit StubLoader(ResourceRegistry& registry): ResourceLoader(registry) {}

    Resource* load(const SourceMetadata& meta, const ResourceSource& source) override { return nullptr; };
};


class LoaderFactory
{
public:
    LoaderFactory(ResourceRegistry& registry, renderer::vulkan::VulkanContext& context);

    ResourceLoader& get(const SourceMetadata& meta);

protected:
    StubLoader stub_loader;
    llvm::DenseMap<ResourceType, std::unique_ptr<ResourceLoader>> loaders;
    renderer::vulkan::VulkanContext& context;
};

} // portal