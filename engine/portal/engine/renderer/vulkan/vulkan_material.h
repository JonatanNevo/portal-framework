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


namespace portal::renderer::vulkan {
class VulkanShaderVariant;
}

namespace portal::renderer::vulkan
{
class VulkanContext;

class VulkanMaterial final : public Material
{
public:
    VulkanMaterial(const MaterialSpecification& spec, const VulkanContext& context);
    ~VulkanMaterial() override;

    void set_pipeline(const Reference<VulkanPipeline>& new_pipeline);
    Reference<VulkanPipeline> get_pipeline() const;

    using Material::set;
    void set(StringId bind_point, const ResourceReference<Texture>& texture) override;
    void set(StringId bind_point, const Reference<Texture>& texture) override;
    void set(StringId bind_point, const Reference<Image>& image) override;
    void set(StringId bind_point, const Reference<ImageView>& image) override;

    using Material::get;
    Reference<Texture> get_texture(StringId bind_point) override;
    Reference<Image> get_image(StringId bind_point) override;
    Reference<ImageView> get_image_view(StringId bind_point) override;
    Reference<ShaderVariant> get_shader() override;
    StringId get_id() override;

    [[nodiscard]] vk::DescriptorSet get_descriptor_set(size_t index) const;

    bool operator==(const VulkanMaterial& other) const;

protected:
    void set_property(StringId bind_point, const reflection::Property& property) override;
    bool get_property(StringId bind_point, reflection::Property& property) const override;

private:
    void allocate_storage();

public:
    struct UniformPointer
    {
        StringId bind_point;
        StringId buffer_name;
        shader_reflection::Uniform uniform;
    };

private:
    MaterialSpecification spec;

    const VulkanDevice& device;
    Reference<VulkanShaderVariant> shader_variant;
    Reference<VulkanPipeline> pipeline;

    std::unordered_map<StringId, UniformPointer> uniforms;
    std::unordered_map<StringId, Reference<BufferDescriptor>> buffers;
    std::unique_ptr<VulkanDescriptorSetManager> descriptor_manager;
};

}
