//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "portal/core/strings/string_id.h"


namespace portal::renderer::vulkan
{
/**
 * @class DescriptorLayoutBuilder
 * @brief Builder for Vulkan descriptor set layouts
 *
 * Fluent API for constructing vk::DescriptorSetLayout from bindings.
 */
class DescriptorLayoutBuilder
{
public:
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    StringId name = INVALID_STRING_ID;

    /**
     * @brief Adds descriptor binding
     * @param binding Binding index
     * @param type Descriptor type
     * @param shader_stages Shader stages accessing binding
     * @param count Array size (default 1)
     * @return Builder reference for chaining
     */
    DescriptorLayoutBuilder& add_binding(size_t binding, vk::DescriptorType type, vk::ShaderStageFlags shader_stages, size_t count = 1);

    /**
     * @brief Sets layout name for debugging
     * @param layout_name Debug name
     * @return Builder reference for chaining
     */
    DescriptorLayoutBuilder& set_name(const StringId& layout_name);

    /** @brief Clears all bindings */
    void clear();

    /**
     * @brief Creates Vulkan descriptor set layout
     * @param device Vulkan device
     * @return Descriptor set layout
     */
    vk::raii::DescriptorSetLayout build(const vk::raii::Device& device);
};
} // portal
