//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/shaders/shader_compiler.h"

namespace portal::renderer::vulkan
{

class VulkanShaderVariant;

class VulkanShader final : public Shader
{
public:
    explicit VulkanShader(const StringId& id, const VulkanContext& context);

    WeakReference<ShaderVariant> get_shader(uint64_t shader_hash) override;

private:
    const VulkanContext& context;
    std::unordered_map<uint64_t, Reference<VulkanShaderVariant>> variant_map;
};


class VulkanShaderVariant final : public ShaderVariant
{
public:
    VulkanShaderVariant(const StringId& name, const VulkanContext& context);
    ~VulkanShaderVariant() override;

    void release();

    StringId get_name() const override;

    std::unordered_map<StringId, vk::WriteDescriptorSet>& get_write_descriptor_sets(size_t set_index);
    vk::DescriptorSetLayout get_descriptor_layout(size_t set_index) const;

    std::vector<vk::DescriptorSetLayout> get_descriptor_layouts() const ;
    const std::vector<vk::PushConstantRange>& get_push_constant_ranges() const;

    const std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration>& get_shader_resources() const override;
    const ShaderReflection& get_reflection() const override;

    const std::vector<vk::PipelineShaderStageCreateInfo>& get_shader_stage_create_infos() const;

protected:
    void load(CompiledShader&& compiled_shader);
    void create_descriptors();


private:
    friend class VulkanShader;

    CompiledShader code;

    StringId name;
    const VulkanDevice& device;

    std::vector<vk::raii::ShaderModule> shader_modules = {};
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos = {};

    std::vector<vk::raii::DescriptorSetLayout> descriptor_layouts;
    std::unordered_map<size_t, std::unordered_map<StringId, vk::WriteDescriptorSet>> write_descriptor_sets;

    std::unordered_map<uint32_t, std::vector<vk::DescriptorPoolSize>> type_counts;

    std::vector<vk::PushConstantRange> push_constant_ranges;
};

}
