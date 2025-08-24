//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "slang-com-ptr.h"

#include "portal/engine/resources/gpu_context.h"
#include "portal/engine/resources/loader/loader.h"
#include "../../shaders/shader.h"

namespace portal::resources
{
class ShaderLoader final : public ResourceLoader
{
public:
    explicit ShaderLoader(ResourceRegistry* registry, const std::shared_ptr<GpuContext>& context);

    bool load(std::shared_ptr<ResourceSource> source) const override;

    void load_default(Ref<Resource>& resource) const override;

protected:
    bool load_precompiled_shader(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader) const;
    bool load_shader(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader) const;

    static void compile_shaders(const std::shared_ptr<ResourceSource>& source, Ref<Shader>& shader);
    // void reflect_shader(slang::ProgramLayout* layout, Ref<Shader>& shader) const;

    // void describe_parameter_block(Shader::Layout& shader_layout, slang::TypeLayoutReflection* parameter_block) const;

private:
    Slang::ComPtr<slang::IGlobalSession> slang_session;
    std::shared_ptr<GpuContext> context;

};
} // portal
