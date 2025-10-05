//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "slang-com-ptr.h"

#include "portal/engine/renderer/vulkan/gpu_context.h"
#include "portal/engine/resources/loader/loader.h"

namespace portal::renderer {
class Shader;
}

namespace portal::resources
{
class ShaderLoader final : public ResourceLoader
{
public:
    explicit ShaderLoader(ResourceRegistry* registry, const std::shared_ptr<renderer::vulkan::GpuContext>& context);

    [[nodiscard]] bool load(std::shared_ptr<ResourceSource> source) const override;

    void load_default(Ref<Resource>& resource) const override;

protected:
    bool load_precompiled_shader(const std::shared_ptr<ResourceSource>& source, Ref<renderer::Shader>& shader) const;

private:
    Slang::ComPtr<slang::IGlobalSession> slang_session;
    std::shared_ptr<renderer::vulkan::GpuContext> context;

};
} // portal
