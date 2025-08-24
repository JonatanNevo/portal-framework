//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/buffer.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/shaders/shader_types.h"

namespace portal
{
namespace resources
{
    class ShaderLoader;
}

namespace vulkan
{
    class VulkanShader;
}

class Shader final: public Resource
{
public:
    explicit Shader(const StringId& id): Resource(id) {}
    void copy_from(Ref<Resource> other) override;

    const std::string& get_entry_point(ShaderStage stage) const;

protected:
    friend class resources::ShaderLoader;
    friend class vulkan::VulkanShader;

    ShaderReflection reflection = {};
    Buffer code = nullptr;
};

} // portal
