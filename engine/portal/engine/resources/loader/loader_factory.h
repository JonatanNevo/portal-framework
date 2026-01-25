//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>

#include "llvm/ADT/DenseMap.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/resources/database/resource_database.h"
#include "loader.h"

namespace portal
{
class Project;
class RendererContext;
class ResourceRegistry;
}

namespace portal::renderer::vulkan
{
class VulkanContext;
}

namespace portal::resources
{
class StubLoader final : public ResourceLoader
{
public:
    explicit StubLoader(ResourceRegistry& registry) : ResourceLoader(registry) {}

    ResourceData load(const SourceMetadata&, Reference<ResourceSource>) override { return {}; };
    void save(ResourceData&) override {};
};


class LoaderFactory
{
public:
    LoaderFactory(const Project& project, ResourceRegistry& registry, const renderer::vulkan::VulkanContext& context);

    ResourceLoader& get(const SourceMetadata& meta);

    static void enrich_metadata(SourceMetadata& meta, const ResourceSource& source);

protected:
    StubLoader stub_loader;
    llvm::DenseMap<ResourceType, std::shared_ptr<ResourceLoader>> loaders;
    const renderer::vulkan::VulkanContext& context;
};
} // portal
