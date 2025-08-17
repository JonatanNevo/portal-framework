//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/buffer.h"
#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal
{
namespace resources {
    class ShaderLoader;
}

class Shader final: public Resource
{
public:
    explicit Shader(const StringId& id): Resource(id) {}
    void copy_from(Ref<Resource> other) override;

    vk::DescriptorSetLayout get_descriptor_layout() const;

    std::optional<vk::PushConstantRange> get_push_constant_range(vk::ShaderStageFlagBits stage) const;
    const std::string& get_entry_point(vk::ShaderStageFlagBits stage) const;

    vk::ShaderModule get_shader_module(vk::ShaderStageFlagBits stage) const;

private:
    friend class resources::ShaderLoader;

    struct ShaderData
    {
        vk::PushConstantRange push_constant_range;
        std::shared_ptr<vk::raii::ShaderModule> shader_module = nullptr;
        std::string entry_point;
    };

    std::shared_ptr<vk::raii::DescriptorSetLayout> descriptor_layout = nullptr;
    std::unordered_map<vk::ShaderStageFlagBits, ShaderData> shader_data;
    Buffer code = nullptr;
};

} // portal
