//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "sub_mesh.h"
#include <ranges>

#include "portal/application/module/renderer/scene/components/material.h"

namespace portal::scene_graph
{
SubMesh::SubMesh(const std::string& name): Component(name)
{}

std::type_index SubMesh::get_type()
{
    return typeid(SubMesh);
}

void SubMesh::set_attribute(const std::string& name, const VertexAttribute& attribute)
{
    vertex_attributes[name] = attribute;
    compute_shader_variant();
}

bool SubMesh::get_attribute(const std::string& name, VertexAttribute& attribute) const
{
    const auto attribute_itr = vertex_attributes.find(name);
    if (attribute_itr == vertex_attributes.end())
        return false;
    attribute = attribute_itr->second;
    return true;
}

void SubMesh::set_material(const Material& material)
{
    this->material = &material;
    compute_shader_variant();
}

const Material* SubMesh::get_material() const
{
    return material;
}

const vulkan::ShaderVariant& SubMesh::get_shader_variant() const
{
    return shader_varint;
}

vulkan::ShaderVariant& SubMesh::get_mut_shader_variant()
{
    return shader_varint;
}

void SubMesh::compute_shader_variant()
{
    shader_varint.clear();

    if (material != nullptr)
    {
        for (const auto& texture_name : material->textures | std::views::keys)
        {
            std::string upper_name;
            upper_name.reserve(texture_name.size());
            std::ranges::transform(texture_name, std::back_inserter(upper_name), [](const char c) { return static_cast<char>(std::toupper(c)); });

            shader_varint.add_define(std::format("HAS_{}", upper_name));
        }
    }

    for (const auto& attribute_name : vertex_attributes | std::views::keys)
    {
        std::string upper_name;
        upper_name.reserve(attribute_name.size());
        std::ranges::transform(attribute_name, std::back_inserter(upper_name), [](const char c) { return static_cast<char>(std::toupper(c)); });

        shader_varint.add_define(std::format("HAS_{}", upper_name));
    }
}
} // portal
