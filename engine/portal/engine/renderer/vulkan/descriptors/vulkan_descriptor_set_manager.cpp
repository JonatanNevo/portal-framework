//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_descriptor_set_manager.h"

#include <ranges>

#include "portal/core/string_utils.h"
#include "portal/core/debug/profile.h"
#include "portal/engine/renderer/shaders/shader.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_storage_buffer.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_uniform_buffer.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"

namespace portal::renderer::vulkan
{

static auto logger = Log::get_logger("Renderer");

DescriptorType to_descriptor_type(const vk::DescriptorType type)
{
    switch (type)
    {
    case vk::DescriptorType::eCombinedImageSampler:
    case vk::DescriptorType::eSampledImage:
        return DescriptorType::CombinedImageSampler;
    case vk::DescriptorType::eStorageImage:
        return DescriptorType::StorageImage;
    case vk::DescriptorType::eUniformBuffer:
        return DescriptorType::UniformBuffer;
    case vk::DescriptorType::eStorageBuffer:
        return DescriptorType::StorageBuffer;
    default:
        break;
    }

    LOGGER_ERROR("Unsupported descriptor type for descriptor_input: {}", vk::to_string(type));
    PORTAL_ASSERT(false, "Unsupported descriptor type");
    return DescriptorType::Unknown;
}

DescriptorResourceType to_descriptor_resource_type(const vk::DescriptorType type)
{
    switch (type)
    {
    case vk::DescriptorType::eCombinedImageSampler:
    case vk::DescriptorType::eSampledImage:
        return DescriptorResourceType::Texture;
    case vk::DescriptorType::eStorageImage:
        return DescriptorResourceType::Image;
    case vk::DescriptorType::eUniformBuffer:
        return DescriptorResourceType::UniformBuffer;
    case vk::DescriptorType::eStorageBuffer:
        return DescriptorResourceType::StorageBuffer;
    default:
        break;
    }

    LOGGER_ERROR("Unsupported descriptor type for descriptor_input: {}", vk::to_string(type));
    PORTAL_ASSERT(false, "Unsupported descriptor type");
    return DescriptorResourceType::Unknown;
}

bool is_compatible_input(DescriptorResourceType input, vk::DescriptorType type)
{
    switch (type)
    {
    case vk::DescriptorType::eCombinedImageSampler:
    case vk::DescriptorType::eSampledImage:
        return input == DescriptorResourceType::Texture || input == DescriptorResourceType::Image || input == DescriptorResourceType::TextureCube;
    case vk::DescriptorType::eStorageImage:
        return input == DescriptorResourceType::Image;
    case vk::DescriptorType::eUniformBuffer:
        return input == DescriptorResourceType::UniformBuffer || input == DescriptorResourceType::UniformBufferSet;
    case vk::DescriptorType::eStorageBuffer:
        return input == DescriptorResourceType::StorageBuffer || input == DescriptorResourceType::StorageBufferSet;
    default:
        return false;
    }
}

VulkanDescriptorSetManager::~VulkanDescriptorSetManager()
{
    descriptor_sets.clear();
    write_descriptors_map.clear();

    descriptor_allocator.clear_pools();
    descriptor_allocator.destroy_pools();
}

VulkanDescriptorSetManager VulkanDescriptorSetManager::create(const DescriptorSetManagerSpecification& spec, const VulkanDevice& device)
{
    // TODO: use raios generated from shader
    std::vector<portal::renderer::vulkan::DescriptorAllocator::PoolSizeRatio> pool_sizes =
    {
        {vk::DescriptorType::eSampler, 10},
        {vk::DescriptorType::eCombinedImageSampler, 10},
        {vk::DescriptorType::eSampledImage, 10},
        {vk::DescriptorType::eStorageImage, 10},
        {vk::DescriptorType::eUniformTexelBuffer, 10},
        {vk::DescriptorType::eStorageTexelBuffer, 10},
        {vk::DescriptorType::eUniformBuffer, 10},
        {vk::DescriptorType::eStorageBuffer, 10},
        {vk::DescriptorType::eUniformBufferDynamic, 10},
        {vk::DescriptorType::eStorageBufferDynamic, 10},
        {vk::DescriptorType::eInputAttachment, 10}
    };

    return VulkanDescriptorSetManager(spec, device, portal::renderer::vulkan::DescriptorAllocator(device.get_handle(), 10 * 3, pool_sizes));
}

std::unique_ptr<VulkanDescriptorSetManager> VulkanDescriptorSetManager::create_unique(
    const DescriptorSetManagerSpecification& spec,
    const VulkanDevice& device
    )
{
    // TODO: use raios generated from shader
    std::vector<portal::renderer::vulkan::DescriptorAllocator::PoolSizeRatio> pool_sizes =
    {
        {vk::DescriptorType::eSampler, 10},
        {vk::DescriptorType::eCombinedImageSampler, 10},
        {vk::DescriptorType::eSampledImage, 10},
        {vk::DescriptorType::eStorageImage, 10},
        {vk::DescriptorType::eUniformTexelBuffer, 10},
        {vk::DescriptorType::eStorageTexelBuffer, 10},
        {vk::DescriptorType::eUniformBuffer, 10},
        {vk::DescriptorType::eStorageBuffer, 10},
        {vk::DescriptorType::eUniformBufferDynamic, 10},
        {vk::DescriptorType::eStorageBufferDynamic, 10},
        {vk::DescriptorType::eInputAttachment, 10}
    };

    return std::unique_ptr<VulkanDescriptorSetManager>(
        new VulkanDescriptorSetManager(
            spec,
            device,
            portal::renderer::vulkan::DescriptorAllocator(device.get_handle(), 10 * 3, pool_sizes)
            )
        );
}


VulkanDescriptorSetManager::VulkanDescriptorSetManager(
    const DescriptorSetManagerSpecification& spec,
    const VulkanDevice& device,
    portal::renderer::vulkan::DescriptorAllocator&& descriptor_allocator
    ) : spec(spec), device(device), descriptor_allocator(std::move(descriptor_allocator))
{
    init();
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<UniformBufferSet>& buffer)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(buffer);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<UniformBuffer>& buffer)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(buffer);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<StorageBufferSet>& buffer)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(buffer);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<StorageBuffer>& buffer)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(buffer);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<Texture>& texture)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(texture);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<Image>& image)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(image);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

void VulkanDescriptorSetManager::set_input(StringId name, const Reference<ImageView>& image)
{
    auto* decl = get_input_declaration(name);
    if (decl)
        input_resources.at(decl->set).at(decl->binding_index).set(image);
    else
        LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
}

Reference<RendererResource> VulkanDescriptorSetManager::get_input(StringId name)
{
    auto* decl = get_input_declaration(name);
    if (decl)
    {
        const auto set_iterator = input_resources.find(decl->set);
        if (set_iterator != input_resources.end())
        {
            const auto binding_iterator = set_iterator->second.find(decl->binding_index);
            if (binding_iterator != set_iterator->second.end())
                return binding_iterator->second.input[0];
        }
    }

    LOGGER_WARN("[{}] Input {} not found", spec.debug_name.string, name.string);
    return nullptr;
}

bool VulkanDescriptorSetManager::is_invalidated(size_t set, size_t binding_index) const
{
    if (invalid_input_resources.contains(set))
    {
        auto& resource = invalid_input_resources.at(set);
        return resource.contains(binding_index);
    }
    return false;
}

bool VulkanDescriptorSetManager::validate()
{
    const auto reflection_descriptor_sets = spec.shader->get_reflection().descriptor_sets;

    const auto end_set = std::min(spec.end_set, reflection_descriptor_sets.size());
    for (size_t set = spec.start_set; set < end_set; ++set)
    {
        if (!reflection_descriptor_sets[set])
            continue;

        if (!input_resources.contains(set))
        {
            LOGGER_ERROR("[{}] No input resources for set {}", spec.debug_name, set);
            return false;
        }

        const auto& set_input_resources = input_resources.at(set);

        [[maybe_unused]] auto& descriptor_set = reflection_descriptor_sets[set];
        auto& write_descriptor_sets = reference_cast<VulkanShaderVariant>(spec.shader)->get_write_descriptor_sets(static_cast<uint32_t>(set));
        for (auto&& [name, write_descriptor] : write_descriptor_sets)
        {
            auto binding = write_descriptor.dstBinding;
            if (!set_input_resources.contains(binding))
            {
                LOGGER_ERROR("[{}] No input resource for {}.{}", spec.debug_name, set, binding);
                LOGGER_ERROR("[{}] Required resource is {} ({})", spec.debug_name, name, vk::to_string(write_descriptor.descriptorType));
                return false;
            }

            const auto& resource = set_input_resources.at(binding);
            if (!is_compatible_input(resource.type, write_descriptor.descriptorType))
            {
                LOGGER_ERROR(
                    "[{}] Required resource is wrong type! {} but needs {}",
                    spec.debug_name,
                    portal::to_string(resource.type),
                    vk::to_string(write_descriptor.descriptorType)
                    );
                return false;
            }

            if (resource.type != DescriptorResourceType::Image && resource.input[0] == nullptr)
            {
                LOGGER_ERROR("[{}] Resource is null! {} ({}.{})", spec.debug_name, name, set, binding);
                return false;
            }
        }
    }

    return true;
}

void VulkanDescriptorSetManager::bake()
{
    // Make sure all resources are present and we can properly bake
    if (!validate())
    {
        LOGGER_ERROR("[{}] Bake - validation failed", spec.debug_name);
        return;
    }

    // If valid, we can create descriptor sets

    auto buffer_sets = get_buffer_sets();
    const size_t descriptor_set_count = spec.frame_in_flights;
    descriptor_sets.resize(descriptor_set_count);

    for (auto& set : descriptor_sets)
        set.clear();

    for (const auto& [set, data] : input_resources)
    {
        for (size_t frame_index = 0; frame_index < descriptor_set_count; ++frame_index)
        {
            auto layout = reference_cast<VulkanShaderVariant>(spec.shader)->get_descriptor_layout(set);

            auto& descriptor_set = descriptor_sets[frame_index].emplace_back(descriptor_allocator.allocate(layout));
            auto& write_descriptor_map = write_descriptors_map[frame_index][set];
            std::vector<std::vector<vk::DescriptorImageInfo>> image_info_storage;
            size_t image_info_index = 0;

            for (const auto& [binding, input] : data)
            {
                auto& stored_write_descriptor = write_descriptor_map[binding];

                auto& write_descriptor = stored_write_descriptor.write_descriptor_set;
                write_descriptor.dstSet = descriptor_set;

                switch (input.type)
                {
                case DescriptorResourceType::UniformBuffer:
                {
                    auto buffer = reference_cast<VulkanUniformBuffer>(input.input[0]);
                    write_descriptor.pBufferInfo = &buffer->get_descriptor_buffer_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pBufferInfo->buffer;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pBufferInfo->buffer == nullptr)
                        invalid_input_resources[set][binding] = input;
                    break;
                }
                case DescriptorResourceType::UniformBufferSet:
                {
                    auto buffer = reference_cast<VulkanUniformBufferSet>(input.input[0]);
                    write_descriptor.pBufferInfo = &reference_cast<VulkanUniformBuffer>(buffer->get(frame_index))->get_descriptor_buffer_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pBufferInfo->buffer;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pBufferInfo->buffer == nullptr)
                        invalid_input_resources[set][binding] = input;
                    break;
                }
                case DescriptorResourceType::StorageBuffer:
                {
                    auto buffer = reference_cast<VulkanStorageBuffer>(input.input[0]);
                    write_descriptor.pBufferInfo = &buffer->get_descriptor_buffer_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pBufferInfo->buffer;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pBufferInfo->buffer == nullptr)
                        invalid_input_resources[set][binding] = input;
                    break;
                }
                case DescriptorResourceType::StorageBufferSet:
                {
                    auto buffer = reference_cast<VulkanStorageBufferSet>(input.input[0]);
                    write_descriptor.pBufferInfo = &reference_cast<VulkanStorageBuffer>(buffer->get(frame_index))->get_descriptor_buffer_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pBufferInfo->buffer;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pBufferInfo->buffer == nullptr)
                        invalid_input_resources[set][binding] = input;
                    break;
                }
                case DescriptorResourceType::Texture:
                {
                    if (input.input.size() > 1)
                    {
                        image_info_storage.emplace_back(input.input.size());
                        for (size_t i = 0; i < input.input.size(); ++i)
                        {
                            auto texture = reference_cast<VulkanTexture>(input.input[i]);
                            image_info_storage[image_info_index][i] = texture->get_descriptor_image_info();
                        }
                        write_descriptor.pImageInfo = image_info_storage[image_info_index].data();
                        image_info_index++;
                    }
                    else
                    {
                        auto texture = reference_cast<VulkanTexture>(input.input[0]);
                        write_descriptor.pImageInfo = &texture->get_descriptor_image_info();
                    }
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pImageInfo->imageView;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pImageInfo->imageView == nullptr)
                        invalid_input_resources[set][binding] = input;

                    break;
                }
                case DescriptorResourceType::TextureCube:
                {
                    auto texture = reference_cast<VulkanTexture>(input.input[0]);
                    write_descriptor.pImageInfo = &texture->get_descriptor_image_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pImageInfo->imageView;

                    // Defer if the resource doesn't exist
                    if (write_descriptor.pImageInfo->imageView == nullptr)
                        invalid_input_resources[set][binding] = input;

                    break;
                }
                case DescriptorResourceType::Image:
                {
                    auto image = reference_cast<VulkanImage>(input.input[0]);
                    if (image == nullptr)
                    {
                        invalid_input_resources[set][binding] = input;
                        break;
                    }

                    write_descriptor.pImageInfo = &image->get_descriptor_image_info();
                    stored_write_descriptor.resource_handles[0] = write_descriptor.pImageInfo->imageView;
                }
                default:
                    LOGGER_ERROR("Invalid input type");
                }
            }

            std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
            for (auto&& [binding, write_descriptor] : write_descriptor_map)
            {
                // Include if valid, otherwise defer (these will be resolved if possible at Prepare stage)
                if (!is_invalidated(set, binding))
                    write_descriptor_sets.emplace_back(write_descriptor.write_descriptor_set);
            }

            if (!write_descriptor_sets.empty())
            {
                device.get_handle().updateDescriptorSets(write_descriptor_sets, {});
            }
        }
    }
}

const shader_reflection::ShaderResourceDeclaration* VulkanDescriptorSetManager::get_input_declaration(const StringId& name) const
{
    if (!input_declarations.contains(name))
        return nullptr;

    return &input_declarations.at(name);
}

void VulkanDescriptorSetManager::invalidate_and_update(const size_t frame_index)
{
    PORTAL_PROF_ZONE();

    // Check for invalidated resources
    for (const auto& [set, inputs] : input_resources)
    {
        for (const auto& [binding, input] : inputs)
        {
            switch (input.type)
            {
            case DescriptorResourceType::UniformBuffer:
            {
                const auto& buffer_info = reference_cast<VulkanUniformBuffer>(input.input[0])->get_descriptor_buffer_info();
                if (buffer_info.buffer != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            case DescriptorResourceType::UniformBufferSet:
            {
                const auto& buffer_info = reference_cast<VulkanUniformBuffer>(
                    reference_cast<VulkanUniformBufferSet>(input.input[0])->get(frame_index)
                    )->get_descriptor_buffer_info();
                if (buffer_info.buffer != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            case DescriptorResourceType::StorageBuffer:
            {
                const auto& buffer_info = reference_cast<VulkanStorageBuffer>(input.input[0])->get_descriptor_buffer_info();
                if (buffer_info.buffer != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            case DescriptorResourceType::StorageBufferSet:
            {
                const auto& buffer_info = reference_cast<VulkanStorageBuffer>(
                    reference_cast<VulkanStorageBufferSet>(input.input[0])->get(frame_index)
                    )->get_descriptor_buffer_info();
                if (buffer_info.buffer != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            case DescriptorResourceType::Texture:
                for (size_t i = 0; i < input.input.size(); i++)
                {
                    auto texture = reference_cast<VulkanTexture>(input.input[i]);
                    // If texture is null put some error texture

                    const auto& image_info = texture->get_descriptor_image_info();
                    if (image_info.imageView != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[i])
                    {
                        invalid_input_resources[set][binding] = input;
                        break;
                    }
                }
                break;
            case DescriptorResourceType::TextureCube:
            {
                const auto& image_info = reference_cast<VulkanTexture>(input.input[0])->get_descriptor_image_info();
                if (image_info.imageView != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            case DescriptorResourceType::Image:
            {
                const auto& image_info = reference_cast<VulkanImage>(input.input[0])->get_descriptor_image_info();
                if (image_info.imageView != write_descriptors_map[frame_index].at(set).at(binding).resource_handles[0])
                    invalid_input_resources[set][binding] = input;
                break;
            }
            default:
                LOGGER_ERROR("Invalid input type");
                break;
            }
        }
    }

    if (invalid_input_resources.empty())
        return;

    for (const auto& [set, data] : invalid_input_resources)
    {
        // Go through every resource here and call vkUpdateDescriptorSets with write descriptors
        // If we don't have valid buffers/images to bind to here, that's an error and needs to be
        // probably handled by putting in some error resources, otherwise we'll crash

        std::vector<vk::WriteDescriptorSet> write_descriptor_to_update;
        write_descriptor_to_update.reserve(data.size());

        std::vector<std::vector<vk::DescriptorImageInfo>> image_info_storage;
        size_t image_info_index = 0;
        for (const auto& [binding, input] : data)
        {
            auto& [write_descriptor_set, resource_handles] = write_descriptors_map[frame_index][set].at(binding);
            switch (input.type)
            {
            case DescriptorResourceType::UniformBuffer:
            {
                auto buffer = reference_cast<VulkanUniformBuffer>(input.input[0]);
                write_descriptor_set.pBufferInfo = &buffer->get_descriptor_buffer_info();
                resource_handles[0] = write_descriptor_set.pBufferInfo->buffer;
            }
            case DescriptorResourceType::UniformBufferSet:
            {
                auto buffer = reference_cast<VulkanUniformBufferSet>(input.input[0]);
                write_descriptor_set.pBufferInfo = &reference_cast<VulkanUniformBuffer>(buffer->get(frame_index))->get_descriptor_buffer_info();
                resource_handles[0] = write_descriptor_set.pBufferInfo->buffer;
                break;
            }
            case DescriptorResourceType::StorageBuffer:
            {
                auto buffer = reference_cast<VulkanStorageBuffer>(input.input[0]);
                write_descriptor_set.pBufferInfo = &buffer->get_descriptor_buffer_info();
                resource_handles[0] = write_descriptor_set.pBufferInfo->buffer;
                break;
            }
            case DescriptorResourceType::StorageBufferSet:
            {
                auto buffer = reference_cast<VulkanStorageBufferSet>(input.input[0]);
                write_descriptor_set.pBufferInfo = &reference_cast<VulkanStorageBuffer>(buffer->get(frame_index))->get_descriptor_buffer_info();
                resource_handles[0] = write_descriptor_set.pBufferInfo->buffer;
                break;
            }
            case DescriptorResourceType::Texture:
            {
                if (input.input.size() > 1)
                {
                    image_info_storage.emplace_back(input.input.size());
                    for (size_t i = 0; i < input.input.size(); ++i)
                    {
                        auto texture = reference_cast<VulkanTexture>(input.input[i]);
                        image_info_storage[image_info_index][i] = texture->get_descriptor_image_info();
                        resource_handles[i] = image_info_storage[image_info_index][i].imageView;
                    }
                    write_descriptor_set.pImageInfo = image_info_storage[image_info_index].data();
                    image_info_index++;
                }
                else
                {
                    auto texture = reference_cast<VulkanTexture>(input.input[0]);
                    write_descriptor_set.pImageInfo = &texture->get_descriptor_image_info();
                    resource_handles[0] = write_descriptor_set.pImageInfo->imageView;
                }
                break;
            }
            case DescriptorResourceType::TextureCube:
            {
                auto texture = reference_cast<VulkanTexture>(input.input[0]);
                write_descriptor_set.pImageInfo = &texture->get_descriptor_image_info();
                resource_handles[0] = write_descriptor_set.pImageInfo->imageView;
                break;
            }
            case DescriptorResourceType::Image:
            {
                auto image = reference_cast<VulkanImage>(input.input[0]);
                write_descriptor_set.pImageInfo = &image->get_descriptor_image_info();
                resource_handles[0] = write_descriptor_set.pImageInfo->imageView;
                break;
            }
            case DescriptorResourceType::Unknown:
                LOGGER_ERROR("Invalid input type");
                break;
            }

            write_descriptor_to_update.emplace_back(write_descriptor_set);
        }

        LOGGER_DEBUG(
            "{} - updating {} descriptors in set {} (frame index = {})",
            spec.shader->get_name(),
            write_descriptor_to_update.size(),
            set,
            frame_index
            );
        device.get_handle().updateDescriptorSets(write_descriptor_to_update, {});
    }

    invalid_input_resources.clear();
}

size_t VulkanDescriptorSetManager::get_first_set_index() const
{
    if (input_resources.empty())
        return std::numeric_limits<size_t>::max();

    return input_resources.begin()->first;
}

const std::vector<vk::raii::DescriptorSet>& VulkanDescriptorSetManager::get_descriptor_sets(size_t frame_index) const
{
    PORTAL_ASSERT(!descriptor_sets.empty(), "DescriptorSets cannot be empty");

    if (frame_index > 0 && descriptor_sets.size() == 1)
        return descriptor_sets.front(); // all frames point to the same descriptor set

    return descriptor_sets[frame_index];
}

VulkanDescriptorSetManager& VulkanDescriptorSetManager::init()
{
    const auto& reflection_descriptor_sets = spec.shader->get_reflection().descriptor_sets;
    const auto frames_in_flight = spec.frame_in_flights;
    write_descriptors_map.resize(frames_in_flight);

    const auto end_set = std::min(spec.end_set, reflection_descriptor_sets.size());
    for (size_t set = spec.start_set; set < end_set; ++set)
    {
        [[maybe_unused]] auto& descriptor_set = reflection_descriptor_sets[set];
        auto& write_descriptor_sets = reference_cast<VulkanShaderVariant>(spec.shader)->get_write_descriptor_sets(set);
        for (auto&& [name, write_descriptor] : write_descriptor_sets)
        {
            const auto binding = write_descriptor.dstBinding;
            input_declarations[name] = {
                .name = name,
                .type = to_descriptor_type(write_descriptor.descriptorType),
                .set = set,
                .binding_index = binding,
                .count = write_descriptor.descriptorCount
            };
            auto& input = input_resources[set][binding];
            input.input.resize(write_descriptor.descriptorCount);
            input.type = to_descriptor_resource_type(write_descriptor.descriptorType);

            // Set default textures
            if (to_descriptor_type(write_descriptor.descriptorType) == DescriptorType::CombinedImageSampler)
            {
                for (size_t i = 0; i < input.input.size(); i++)
                {
                    input.input[i] = spec.default_texture;
                }
            }

            //     else if (inputDecl.Type == RenderPassInputType::ImageSampler3D)
            //     {
            //         for (size_t i = 0; i < input.Input.size(); i++)
            //             input.Input[i] = Renderer::GetBlackCubeTexture();
            //     }

            for (size_t frame_index = 0; frame_index < frames_in_flight; ++frame_index)
                write_descriptors_map[frame_index][set][binding] = {.write_descriptor_set = write_descriptor,
                                                                    .resource_handles = std::vector<void*>(write_descriptor.descriptorCount)};
        }
    }

    return *this;
}

std::set<size_t> VulkanDescriptorSetManager::get_buffer_sets()
{
    std::set<size_t> sets_with_buffers;

    // Find all descriptor sets that have either UniformBufferSet or StorageBufferSet descriptors
    for (const auto& [set, resources] : input_resources)
    {
        for (const auto& input : resources | std::views::values)
        {
            if (input.type == DescriptorResourceType::UniformBufferSet || input.type == DescriptorResourceType::StorageBufferSet)
            {
                sets_with_buffers.insert(set);
                break;
            }
        }
    }

    return sets_with_buffers;
}

}
