//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "loader.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal
{
    class ResourceRegistry;
}

namespace portal::resources
{
class GpuContext;

class LoaderFactory
{
public:
    void initialize(ResourceRegistry* registry, const std::shared_ptr<GpuContext>& context);
    void shutdown();
    std::shared_ptr<ResourceLoader> get(ResourceType type);

private:
    std::shared_ptr<GpuContext> gpu_context;
    std::shared_ptr<StubLoader> stub_loader;
};

}
