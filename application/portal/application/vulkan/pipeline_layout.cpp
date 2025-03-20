//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "pipeline_layout.h"

#include <ranges>

#include "portal/application/vulkan/descriptor_set_layout.h"
#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
PipelineLayout::PipelineLayout(Device& device, const std::vector<ShaderModule*>& shader_modules)
    : VulkanResource(nullptr, &device), shader_modules(shader_modules)
{
    // Collect and combine all the shader resources from each of the shader modules
    // Collate them all into a map that is indexed by the name of the resource
    for (const auto* shader_module : shader_modules)
    {
        for (const auto& shader_resource : shader_module->get_resources())
        {
            std::string key = shader_resource.name;

            // Since 'Input' and 'Output' resources can have the same name, we modify the key string
            if (shader_resource.type == ShaderResourceType::Input || shader_resource.type == ShaderResourceType::Output)
            {
                key = std::format("{}_{}", static_cast<uint32_t>(shader_resource.stages), key);
            }

            auto it = shader_resources.find(key);
            if (it != shader_resources.end())
            {
                // Append stage flags if resource already exists
                it->second.stages |= shader_resource.stages;
            }
            else
            {
                // Create a new entry in the map
                shader_resources.emplace(key, shader_resource);
            }
        }
    }

    // Sift through the map of name indexed shader resources
    // Separate them into their respective sets
    for (const auto& shader_resource : shader_resources | std::views::values)
    {
        // Find binding by set index in the map.
        auto it2 = shader_sets.find(shader_resource.set);

        if (it2 != shader_sets.end())
        {
            // Add resource to the found set index
            it2->second.push_back(shader_resource);
        }
        else
        {
            // Create a new set index and with the first resource
            shader_sets.emplace(shader_resource.set, std::vector{shader_resource});
        }
    }

    // Create a descriptor set layout for each shader set in the shader modules
    for (auto& [set_index, set_resources] : shader_sets)
    {
        descriptor_set_layouts.emplace_back(
            &get_device().get_resource_cache().request_descriptor_set_layout(set_index, shader_modules, set_resources)
        );
    }

    // Collect all the descriptor set layout handles, maintaining set order
    std::vector<vk::DescriptorSetLayout> descriptor_set_layout_handles;
    for (const auto& descriptor_set_layout : descriptor_set_layouts)
    {
        descriptor_set_layout_handles.push_back(descriptor_set_layout ? descriptor_set_layout->get_handle() : nullptr);
    }

    // Collect all the push constant shader resources
    std::vector<vk::PushConstantRange> push_constant_ranges;
    for (auto& push_constant_resource : get_resources(ShaderResourceType::PushConstant))
    {
        push_constant_ranges.push_back({push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size});
    }

    const vk::PipelineLayoutCreateInfo create_info({}, descriptor_set_layout_handles, push_constant_ranges);
    // Create the Vulkan pipeline layout handle
    set_handle(device.get_handle().createPipelineLayout(create_info));
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : VulkanResource(std::move(other)),
      shader_modules(std::move(other.shader_modules)),
      shader_resources(std::move(other.shader_resources)),
      shader_sets(std::move(other.shader_sets)),
      descriptor_set_layouts(std::move(other.descriptor_set_layouts))
{}

PipelineLayout::~PipelineLayout()
{
    // Destroy pipeline layout
    if (has_handle())
    {
        get_device().get_handle().destroyPipelineLayout(get_handle());
    }
}

const DescriptorSetLayout& PipelineLayout::get_descriptor_set_layout(uint32_t set_index) const
{
    const auto it = std::ranges::find_if(
        descriptor_set_layouts,
        [&set_index](auto const* descriptor_set_layout) { return descriptor_set_layout->get_index() == set_index; }
    );
    if (it == descriptor_set_layouts.end())
    {
        throw std::runtime_error(std::format("Couldn't find descriptor set layout at set index {}", set_index));
    }
    return **it;
}

vk::ShaderStageFlags PipelineLayout::get_push_constant_range_stage(const uint32_t size, const uint32_t offset) const
{
    vk::ShaderStageFlags stages;
    for (const auto& push_constant_resource : get_resources(ShaderResourceType::PushConstant))
    {
        if (push_constant_resource.offset <= offset && offset + size <= push_constant_resource.offset + push_constant_resource.size)
        {
            stages |= push_constant_resource.stages;
        }
    }
    return stages;
}

std::vector<ShaderResource> PipelineLayout::get_resources(const ShaderResourceType& type, const vk::ShaderStageFlagBits stage) const
{
    std::vector<ShaderResource> found_resources;
    for (const auto& resources : shader_resources | std::views::values)
    {
        auto& shader_resource = resources;
        if (shader_resource.type == type || type == ShaderResourceType::All)
        {
            if (shader_resource.stages == stage || stage == vk::ShaderStageFlagBits::eAll)
            {
                found_resources.push_back(shader_resource);
            }
        }
    }
    return found_resources;
}

const std::vector<ShaderModule*>& PipelineLayout::get_shader_modules() const
{
    return shader_modules;
}

std::unordered_map<uint32_t, std::vector<ShaderResource>> PipelineLayout::get_shader_sets() const
{
    return shader_sets;
}

bool PipelineLayout::has_descriptor_set_layout(const uint32_t set_index) const
{
    return set_index < descriptor_set_layouts.size();
}
} // portal
