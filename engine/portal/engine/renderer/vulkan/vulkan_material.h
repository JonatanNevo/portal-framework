//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/descriptors/descriptor_set_manager.h"
#include "portal/engine/renderer/material/material.h"
#include "portal/engine/renderer/vulkan/vulkan_pipeline.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_descriptor_set_manager.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_uniform_buffer.h"


namespace portal::renderer::vulkan
{
class VulkanShaderVariant;
}

namespace portal::renderer::vulkan
{
class VulkanContext;

/**
 * @class VulkanMaterial
 * @brief Vulkan material with descriptor set management and uniform storage
 *
 * Manages shader parameter binding through descriptor sets and uniform buffers.
 * Automatically maps shader uniforms to CPU storage and GPU descriptors.
 */
class VulkanMaterial final : public Material
{
public:
    /**
     * @brief Constructs Vulkan material
     * @param properties Material configuration
     * @param context Renderer context
     */
    VulkanMaterial(const MaterialProperties& properties, const RendererContext& context);
    ~VulkanMaterial() override;

    /**
     * @brief Sets rendering pipeline
     * @param new_pipeline Vulkan pipeline
     */
    void set_pipeline(const Reference<VulkanPipeline>& new_pipeline);

    /** @brief Gets rendering pipeline */
    [[nodiscard]] Reference<VulkanPipeline> get_pipeline() const;

    using Material::set;
    /** @brief Binds texture (resource reference) */
    void set(StringId bind_point, const ResourceReference<Texture>& texture) override;

    /** @brief Binds texture */
    void set(StringId bind_point, const Reference<Texture>& texture) override;

    /** @brief Binds image */
    void set(StringId bind_point, const Reference<Image>& image) override;

    /** @brief Binds image view */
    void set(StringId bind_point, const Reference<ImageView>& image) override;

    using Material::get;
    /** @brief Gets bound texture */
    Reference<Texture> get_texture(StringId bind_point) override;

    /** @brief Gets bound image */
    Reference<Image> get_image(StringId bind_point) override;

    /** @brief Gets bound image view */
    Reference<ImageView> get_image_view(StringId bind_point) override;

    /** @brief Gets material shader */
    Reference<ShaderVariant> get_shader() override;

    /**
     * @brief Gets descriptor set at index
     * @param index Descriptor set index
     * @return Vulkan descriptor set
     */
    [[nodiscard]] vk::DescriptorSet get_descriptor_set(size_t index) const;

    /** @brief Equality comparison */
    bool operator==(const VulkanMaterial& other) const;

protected:
    /** @brief Sets property by reflection */
    void set_property(StringId bind_point, const reflection::Property& property) override;

    /** @brief Gets property by reflection */
    bool get_property(StringId bind_point, reflection::Property& property) const override;

private:
    /** @brief Allocates CPU storage for uniforms */
    void allocate_storage();

public:
    /**
     * @struct UniformPointer
     * @brief Maps bind point to shader uniform location
     */
    struct UniformPointer
    {
        StringId bind_point;
        StringId buffer_name;
        shader_reflection::Uniform uniform;
    };

private:
    MaterialProperties properties;

    const VulkanDevice& device;
    Reference<VulkanShaderVariant> shader_variant;
    Reference<VulkanPipeline> pipeline;

    std::unordered_map<StringId, UniformPointer> uniforms;
    std::unordered_map<StringId, Reference<BufferDescriptor>> buffers;
    std::unique_ptr<VulkanDescriptorSetManager> descriptor_manager;
};
}
