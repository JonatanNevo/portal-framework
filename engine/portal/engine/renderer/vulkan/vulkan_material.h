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
    VulkanMaterial() = default;
    ~VulkanMaterial() override;

    void initialize(const MaterialSpecification& new_spec, const Ref<VulkanContext>& context);

    void set_pipeline(const Ref<VulkanPipeline>& new_pipeline);
    Ref<VulkanPipeline> get_pipeline() const;

    using Material::set;
    void set(StringId bind_point, Ref<Texture> texture) override;
    void set(StringId bind_point, Ref<Image> image) override;
    void set(StringId bind_point, Ref<ImageView> image) override;

    using Material::get;
    Ref<Texture> get_texture(StringId bind_point) override;
    Ref<Image> get_image(StringId bind_point) override;
    Ref<ImageView> get_image_view(StringId bind_point) override;
    Ref<ShaderVariant> get_shader() override;
    StringId get_id() override;

    vk::DescriptorSet get_descriptor_set(size_t index);

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

    StringId id;
    Ref<VulkanDevice> device;
    Ref<VulkanShaderVariant> shader_variant;
    Ref<VulkanPipeline> pipeline;

    std::unordered_map<StringId, UniformPointer> uniforms;
    std::unordered_map<StringId, Ref<BufferDescriptor>> buffers;
    VulkanDescriptorSetManager descriptor_manager{};
};

}
