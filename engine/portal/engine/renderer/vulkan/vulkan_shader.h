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

/**
 * @class VulkanShader
 * @brief Vulkan shader variant manager
 *
 * Manages compiled shader variants, creating VulkanShaderVariant instances
 * for each unique permutation.
 */
class VulkanShader final : public Shader
{
public:
    /**
     * @brief Constructs Vulkan shader
     * @param id Shader ID
     * @param context Vulkan context
     */
    explicit VulkanShader(const StringId& id, const VulkanContext& context);
    ~VulkanShader() override;

    /**
     * @brief Gets shader variant by hash
     * @param shader_hash Permutation hash
     * @return Shader variant reference
     */
    WeakReference<ShaderVariant> get_shader(uint64_t shader_hash) override;

private:
    const VulkanContext& context;
    std::unordered_map<uint64_t, Reference<VulkanShaderVariant>> variant_map;
};


/**
 * @class VulkanShaderVariant
 * @brief Vulkan shader modules with descriptor layouts and push constants
 *
 * Wraps compiled shader bytecode in vk::ShaderModule, creates descriptor set layouts
 * from reflection data, and provides pipeline configuration.
 */
class VulkanShaderVariant final : public ShaderVariant
{
public:
    /**
     * @brief Constructs Vulkan shader variant
     * @param name Variant name
     * @param context Vulkan context
     */
    VulkanShaderVariant(const StringId& name, const VulkanContext& context);
    ~VulkanShaderVariant() override;

    /** @brief Releases Vulkan resources */
    void release();

    /** @brief Gets variant name */
    StringId get_name() const override;

    /**
     * @brief Gets write descriptor sets for binding
     * @param set_index Descriptor set index
     * @return Map of descriptor writes by name
     */
    std::unordered_map<StringId, vk::WriteDescriptorSet>& get_write_descriptor_sets(size_t set_index);

    /**
     * @brief Gets descriptor set layout
     * @param set_index Descriptor set index
     * @return Vulkan descriptor set layout
     */
    vk::DescriptorSetLayout get_descriptor_layout(size_t set_index) const;

    /** @brief Gets all descriptor set layouts */
    std::vector<vk::DescriptorSetLayout> get_descriptor_layouts() const;

    /** @brief Gets push constant ranges */
    const std::vector<vk::PushConstantRange>& get_push_constant_ranges() const;

    /** @brief Gets shader resource declarations */
    const std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration>& get_shader_resources() const override;

    /** @brief Gets shader reflection data */
    const ShaderReflection& get_reflection() const override;

    /** @brief Gets pipeline shader stage create infos */
    const std::vector<vk::PipelineShaderStageCreateInfo>& get_shader_stage_create_infos() const;

protected:
    /**
     * @brief Loads compiled shader and creates Vulkan resources
     * @param compiled_shader Compiled bytecode and reflection
     */
    void load(CompiledShader&& compiled_shader);

    /** @brief Creates descriptor set layouts from reflection */
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
