//
// Created by Jonatan Nevo on 11/03/2025.
//

#pragma once

#include "portal/application/module/renderer/scene/component.h"
#include "portal/application/vulkan/shaders/shader_module.h"

namespace portal::vulkan
{
class Buffer;
}

namespace portal::scene_graph
{
class Material;

struct VertexAttribute
{
    vk::Format format = vk::Format::eUndefined;
    std::uint32_t stride = 0;
    std::uint32_t offset = 0;
};


class SubMesh final : public Component
{
public:
    explicit SubMesh(const std::string& name = {});
    std::type_index get_type() override;

    vk::IndexType index_type{};
    std::uint32_t index_offset = 0;
    std::uint32_t vertices_count = 0;
    std::uint32_t vertex_indices = 0;

    std::unordered_map<std::string, vulkan::Buffer> vertex_buffers;
    std::unique_ptr<vulkan::Buffer> index_buffer;

    void set_attribute(const std::string &name, const VertexAttribute &attribute);
    bool get_attribute(const std::string &name, VertexAttribute &attribute) const;

    void set_material(const Material &material);
    const Material *get_material() const;

    const vulkan::ShaderVariant &get_shader_variant() const;
    vulkan::ShaderVariant &get_mut_shader_variant();

private:
    void compute_shader_variant();


    std::unordered_map<std::string, VertexAttribute> vertex_attributes;
    const Material *material{nullptr};
    vulkan::ShaderVariant shader_varint;
};
} // portal
