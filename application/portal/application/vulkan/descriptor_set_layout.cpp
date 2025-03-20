//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "descriptor_set_layout.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/physical_device.h"
#include "shaders/shader_module.h"

namespace portal::vulkan
{
inline vk::DescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic)
{
    switch (resource_type)
    {
    case ShaderResourceType::InputAttachment:
        return vk::DescriptorType::eInputAttachment;
    case ShaderResourceType::Image:
        return vk::DescriptorType::eSampledImage;
    case ShaderResourceType::ImageSampler:
        return vk::DescriptorType::eCombinedImageSampler;
    case ShaderResourceType::ImageStorage:
        return vk::DescriptorType::eStorageImage;
    case ShaderResourceType::Sampler:
        return vk::DescriptorType::eSampler;
    case ShaderResourceType::BufferUniform:
        if (dynamic)
            return vk::DescriptorType::eUniformBufferDynamic;
        return vk::DescriptorType::eUniformBuffer;
    case ShaderResourceType::BufferStorage:
        if (dynamic)
            return vk::DescriptorType::eStorageBufferDynamic;
        return vk::DescriptorType::eStorageBuffer;
    default:
        throw std::runtime_error("No conversion possible for the shader resource type.");
    }
}

inline bool validate_binding(const vk::DescriptorSetLayoutBinding& binding, const std::vector<vk::DescriptorType>& blacklist)
{
    return std::ranges::find_if(blacklist, [&binding](const auto& type) { return type == binding.descriptorType; }) == blacklist.end();
}

inline bool validate_flags(
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
    const std::vector<vk::DescriptorBindingFlagsEXT>& flags
)
{
    // Assume bindings are valid if there are no flags
    if (flags.empty())
    {
        return true;
    }

    // Binding count has to equal flag count as its a 1:1 mapping
    if (bindings.size() != flags.size())
    {
        LOG_CORE_ERROR_TAG("Vulkan", "Binding count has to be equal to flag count.");
        return false;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DescriptorSetLayout::DescriptorSetLayout(
    Device& device,
    const uint32_t set_index,
    const std::vector<ShaderModule*>& shader_modules,
    const std::vector<ShaderResource>& resource_set
): VulkanResource(nullptr, &device), set_index(set_index), shader_modules(shader_modules)
{
    // NOTE: `shader_modules` is passed in mainly for hashing their handles in `request_resource`.
    //        This way, different pipelines (with different shaders / shader variants) will get
    //        different descriptor set layouts (incl. appropriate name -> binding lookups)
    for (const auto& resource : resource_set)
    {
        // Skip shader resources without a binding point
        if (resource.type == ShaderResourceType::Input ||
            resource.type == ShaderResourceType::Output ||
            resource.type == ShaderResourceType::PushConstant ||
            resource.type == ShaderResourceType::SpecializationConstant)
        {
            continue;
        }

        // Convert from ShaderResourceType to vk::DescriptorType.
        const auto descriptor_type = find_descriptor_type(resource.type, resource.mode == ShaderResourceMode::Dynamic);

        if (resource.mode == ShaderResourceMode::UpdateAfterBind)
        {
            binding_flags.push_back(vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind);
        }
        else
        {
            // When creating a descriptor set layout, if we give a structure to create_info.pNext, each binding needs to have a binding flag
            // (pBindings[i] uses the flags in pBindingFlags[i])
            // Adding 0 ensures the bindings that don't use any flags are mapped correctly.
            binding_flags.push_back({});
        }

        // Convert ShaderResource to VkDescriptorSetLayoutBinding
        vk::DescriptorSetLayoutBinding layout_binding{};
        layout_binding.binding = resource.binding;
        layout_binding.descriptorCount = resource.array_size;
        layout_binding.descriptorType = descriptor_type;
        layout_binding.stageFlags = resource.stages;
        bindings.push_back(layout_binding);

        // Store mapping between binding and the binding point
        bindings_lookup.emplace(resource.binding, layout_binding);
        binding_flags_lookup.emplace(resource.binding, binding_flags.back());
        resources_lookup.emplace(resource.name, resource.binding);
    }

    vk::DescriptorSetLayoutCreateInfo create_info({}, bindings);

    vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_info{};
    if (std::ranges::any_of(resource_set, [](const auto& resource) { return resource.mode == ShaderResourceMode::UpdateAfterBind; }))
    {
        // Spec states you can't have ANY dynamic resources if you have one of the bindings set to update-after-bind
        if (std::ranges::any_of(resource_set, [](const auto& resource) { return resource.mode == ShaderResourceMode::Dynamic; }))
            throw std::runtime_error(
                "Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind."
            );

        if (!validate_flags(bindings, binding_flags))
            throw std::runtime_error("Invalid binding, couldn't create descriptor set layout.");

        binding_flags_info.bindingCount = binding_flags.size();
        binding_flags_info.pBindingFlags = binding_flags.data();

        create_info.pNext = &binding_flags_info;
        create_info.flags |= std::ranges::find(binding_flags, vk::DescriptorBindingFlagBits::eUpdateAfterBind) != binding_flags.end()
                                 ? vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool
                                 : vk::DescriptorSetLayoutCreateFlags{};
    }

    set_handle(device.get_handle().createDescriptorSetLayout(create_info));
    if (!has_handle())
        throw std::runtime_error("Failed to create descriptor set layout");
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : VulkanResource(std::move(other)),
      shader_modules{other.shader_modules},
      set_index{other.set_index},
      bindings{std::move(other.bindings)},
      binding_flags{std::move(other.binding_flags)},
      bindings_lookup{std::move(other.bindings_lookup)},
      binding_flags_lookup{std::move(other.binding_flags_lookup)},
      resources_lookup{std::move(other.resources_lookup)} {}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (has_handle())
        get_device().get_handle().destroyDescriptorSetLayout(get_handle());
}

uint32_t DescriptorSetLayout::get_index() const
{
    return set_index;
}

const std::vector<vk::DescriptorSetLayoutBinding>& DescriptorSetLayout::get_bindings() const
{
    return bindings;
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayout::get_layout_binding(const uint32_t binding_index) const
{
    const auto it = bindings_lookup.find(binding_index);
    if (it == bindings_lookup.end())
        return nullptr;
    return std::make_unique<vk::DescriptorSetLayoutBinding>(it->second);
}

std::unique_ptr<vk::DescriptorSetLayoutBinding> DescriptorSetLayout::get_layout_binding(const std::string& name) const
{
    const auto it = resources_lookup.find(name);
    if (it == resources_lookup.end())
        return nullptr;
    return get_layout_binding(it->second);
}

const std::vector<vk::DescriptorBindingFlagsEXT>& DescriptorSetLayout::get_binding_flags() const
{
    return binding_flags;
}

vk::DescriptorBindingFlagsEXT DescriptorSetLayout::get_layout_binding_flag(const uint32_t binding_index) const
{
    const auto it = binding_flags_lookup.find(binding_index);
    if (it == binding_flags_lookup.end())
        return {};
    return it->second;
}

const std::vector<ShaderModule*>& DescriptorSetLayout::get_shader_modules() const
{
    return shader_modules;
}
} // portal
