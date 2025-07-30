//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/resources/resources/resource.h"

namespace portal::resources
{
class ResourceSource;

class ResourceLoader
{
public:
    virtual ~ResourceLoader() = default;

    /**
     * Initializes the loader with a given resource source, does not load or block.
     * @param source
     */
    virtual void init(std::shared_ptr<ResourceSource> source) = 0;

    /**
     * Loads the resource from the source provided in the constructor.
     * This function will block until the resource is loaded.
     * @param resource The reference to load into.
     */
    virtual bool load(const Ref<Resource>& resource) = 0;

    /**
     * Loads instantly the default resource for the given resource type.
     * @param resource The reference to load the default resource into.
     */
    virtual void load_default(const Ref<Resource>& resource) const = 0;

    /**
     * Returns the signature of the resources present in the source provided in the `init` function.
     */
    virtual std::vector<ResourceSignature> get_signature() const = 0;
};

class StubLoader final : public ResourceLoader
{
public:
    void init(std::shared_ptr<ResourceSource>) override {};
    bool load(const Ref<Resource>&) override { return false; };
    void load_default(const Ref<Resource>&) const override {};
    std::vector<ResourceSignature> get_signature() const override { return {}; };

};
}
