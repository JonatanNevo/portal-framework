//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "loader.h"
#include "portal/engine/resources/source/resource_source.h"

namespace portal::resources
{
    class GpuContext;
}

namespace portal::resources
{

class LoaderFactory
{
public:
    void initialize(const std::shared_ptr<GpuContext>& context);
    std::shared_ptr<ResourceLoader> get(SourceMetadata metadata);

private:
    std::shared_ptr<ResourceLoader> get_texture_loader(SourceMetadata metadata);

private:
    std::shared_ptr<GpuContext> gpu_context;
    std::shared_ptr<StubLoader> stub_loader;
};

}
