//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/resources/resource.h"

namespace portal
{
class ResourceRegistry;
}


namespace portal::resources
{
class ResourceSource;

class ResourceLoader
{
public:
    explicit ResourceLoader(ResourceRegistry* registry): registry(registry) { PORTAL_ASSERT(registry, "Resource registry is null"); };
    virtual ~ResourceLoader() = default;


    virtual void initialize() {};

    /**
     * Loads the resource from a source
     * This function will block until the resource is loaded.
     * @param source The source of the resource
     */
    virtual bool load(std::shared_ptr<ResourceSource> source) const = 0;

    /**
     * Loads instantly the default resource for the given resource type.
     * @param resource The reference to load the default resource into.
     */
    virtual void load_default(Ref<Resource>& resource) const = 0;

protected:
    ResourceRegistry* registry;
};

class StubLoader final : public ResourceLoader
{
public:
    explicit StubLoader(ResourceRegistry* registry) : ResourceLoader(registry) {}
    bool load(std::shared_ptr<ResourceSource>) const override { return false; };
    void load_default(Ref<Resource>&) const override {};
};
}
