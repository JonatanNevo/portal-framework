//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/reference.h"

namespace portal
{
class ArchiveObject;
class Resource;
struct SourceMetadata;
class ResourceRegistry;
}

namespace portal::resources
{
class ResourceSource;

class ResourceLoader
{
public:
    explicit ResourceLoader(ResourceRegistry& registry) : registry(registry) {}
    virtual ~ResourceLoader() = default;

    /**
     * Loads the resource from a source
     * This function will block until the resource is loaded.
     * If the resource requires additional resources, the loader might trigger additional resource requests from the registry
     *
     * @note The returned resource pointer should be heap-allocated and will be managed by the registry
     *
     * @param meta The source metadata
     * @param source The source of the resource
     * @return A heap-allocated pointer to the loaded resource, nullptr if any error occurred
     */
    virtual Reference<Resource> load(const SourceMetadata& meta, const ResourceSource& source) = 0;

protected:
    ResourceRegistry& registry;
};

}
