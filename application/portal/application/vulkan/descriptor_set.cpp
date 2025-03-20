//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "descriptor_set.h"

#include "portal/application/vulkan/descriptor_pool.h"
#include "portal/application/vulkan/device.h"
#include "resources/hashing.h"

namespace portal::vulkan
{
DescriptorSet::DescriptorSet(
    Device& device,
    const DescriptorSetLayout& descriptor_set_layout,
    DescriptorPool& descriptor_pool,
    const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
    const BindingMap<vk::DescriptorImageInfo>& image_infos
): VulkanResource(descriptor_pool.allocate(), &device),
   descriptor_set_layout(descriptor_set_layout),
   descriptor_pool(descriptor_pool),
   buffer_infos(buffer_infos),
   image_infos(image_infos)
{
    prepare();
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
    : VulkanResource(std::move(other)),
      descriptor_set_layout(std::move(other.descriptor_set_layout)),
      descriptor_pool(other.descriptor_pool),
      buffer_infos(std::exchange(other.buffer_infos, {})),
      image_infos(std::exchange(other.image_infos, {})),
      write_descriptor_sets(std::exchange(other.write_descriptor_sets, {})),
      updated_bindings(std::exchange(other.updated_bindings, {}))
{}


void DescriptorSet::reset(const BindingMap<vk::DescriptorBufferInfo>& new_buffer_infos, const BindingMap<vk::DescriptorImageInfo>& new_image_infos)
{
    if (!new_buffer_infos.empty() || !new_image_infos.empty())
    {
        buffer_infos = new_buffer_infos;
        image_infos = new_image_infos;
    }
    else
        LOG_CORE_WARN_TAG("Vulkan", "Calling reset on Descriptor Set with no new buffer infos and no new image infos.");

    write_descriptor_sets.clear();
    updated_bindings.clear();

    prepare();
}

void DescriptorSet::update(const std::vector<uint32_t>& bindings_to_update)
{
    std::vector<vk::WriteDescriptorSet> write_operations;
    std::vector<size_t> write_operation_hashes;

    // If the 'bindings_to_update' vector is empty, we want to write to all the bindings
    // (but skipping all to-update bindings that haven't been written yet)
    if (bindings_to_update.empty())
    {
        for (size_t i = 0; i < write_descriptor_sets.size(); i++)
        {
            const auto& write_operation = write_descriptor_sets[i];
            size_t write_operation_hash = 0;
            hash_param(write_operation_hash, write_operation);

            auto update_pair_it = updated_bindings.find(write_operation.dstBinding);
            if (update_pair_it == updated_bindings.end() || update_pair_it->second != write_operation_hash)
            {
                write_operations.push_back(write_operation);
                write_operation_hashes.push_back(write_operation_hash);
            }
        }
    }
    else
    {
        // Otherwise we want to update the binding indices present in the 'bindings_to_update' vector.
        // (again, skipping those to update but not updated yet)
        for (size_t i = 0; i < write_descriptor_sets.size(); i++)
        {
            const auto& write_operation = write_descriptor_sets[i];
            if (std::ranges::find(bindings_to_update, write_operation.dstBinding) != bindings_to_update.end())
            {
                size_t write_operation_hash = 0;
                hash_param(write_operation_hash, write_operation);

                auto update_pair_it = updated_bindings.find(write_operation.dstBinding);
                if (update_pair_it == updated_bindings.end() || update_pair_it->second != write_operation_hash)
                {
                    write_operations.push_back(write_operation);
                    write_operation_hashes.push_back(write_operation_hash);
                }
            }
        }
    }

    // Perform the Vulkan call to update the DescriptorSet by executing the write operations
    if (!write_operations.empty())
        apply_writes();

    // Store the bindings from the write operations that were executed by vkUpdateDescriptorSets (and their hash)
    // to prevent overwriting by future calls to "update()"
    for (size_t i = 0; i < write_operations.size(); i++)
    {
        updated_bindings[write_operations[i].dstBinding] = write_operation_hashes[i];
    }
}

void DescriptorSet::apply_writes() const
{
    get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

const DescriptorSetLayout& DescriptorSet::get_layout() const
{
    return descriptor_set_layout;
}

BindingMap<vk::DescriptorBufferInfo>& DescriptorSet::get_buffer_infos()
{
    return buffer_infos;
}

BindingMap<vk::DescriptorImageInfo>& DescriptorSet::get_image_infos()
{
    return image_infos;
}

void DescriptorSet::prepare()
{
    // We don't want to prepare twice during the life cycle of a Descriptor Set
    if (!write_descriptor_sets.empty())
    {
        LOG_CORE_WARN_TAG("Vulkan", "Calling prepare on Descriptor Set that has already been prepared.");
        return;
    }

    // Iterate over all buffer bindings
    for (auto& [binding_index, buffer_bindings] : buffer_infos)
    {
        if (const auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
        {
            // Iterate over all binding buffers in array
            for (auto& [array_element, buffer_info] : buffer_bindings)
            {
                const size_t uniform_buffer_range_limit = get_device().get_gpu().get_properties().limits.maxUniformBufferRange;
                const size_t storage_buffer_range_limit = get_device().get_gpu().get_properties().limits.maxStorageBufferRange;

                size_t buffer_range_limit = buffer_info.range;
                if ((binding_info->descriptorType == vk::DescriptorType::eUniformBuffer || binding_info->descriptorType ==
                    vk::DescriptorType::eUniformBufferDynamic) && buffer_range_limit > uniform_buffer_range_limit)
                {
                    LOG_CORE_ERROR_TAG(
                        "Vulkan",
                        "Set {} binding {} cannot be updated: buffer size {} exceeds the uniform buffer range limit {}",
                        descriptor_set_layout.get_index(),
                        binding_index,
                        buffer_info.range,
                        uniform_buffer_range_limit
                    );
                    buffer_range_limit = uniform_buffer_range_limit;
                }
                else if ((binding_info->descriptorType == vk::DescriptorType::eStorageBuffer || binding_info->descriptorType ==
                    vk::DescriptorType::eStorageBufferDynamic) && buffer_range_limit > storage_buffer_range_limit)
                {
                    LOG_CORE_ERROR_TAG(
                        "Vulkan",
                        "Set {} binding {} cannot be updated: buffer size {} exceeds the storage buffer range limit {}",
                        descriptor_set_layout.get_index(),
                        binding_index,
                        buffer_info.range,
                        storage_buffer_range_limit
                    );
                    buffer_range_limit = storage_buffer_range_limit;
                }
                // Clip the buffers range to the limit if one exists as otherwise we will receive a Vulkan validation error
                buffer_info.range = buffer_range_limit;

                vk::WriteDescriptorSet write_descriptor_set(
                    get_handle(),
                    binding_index,
                    array_element,
                    1,
                    binding_info->descriptorType,
                    nullptr,
                    &buffer_info,
                    nullptr
                );
                write_descriptor_sets.push_back(write_descriptor_set);
            }
        }
        else
        {
            LOG_CORE_ERROR_TAG("Vulkan", "Shader layout set does not use buffer binding at #{}", binding_index);
        }
    }

    for (const auto& [binding_index, image_bindings] : image_infos)
    {
        if (const auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
        {
            // Iterate over all binding images in array
            for (const auto& [array_element, image_info] : image_bindings)
            {
                vk::WriteDescriptorSet write_descriptor_set(
                    get_handle(),
                    binding_index,
                    array_element,
                    1,
                    binding_info->descriptorType,
                    &image_info,
                    nullptr,
                    nullptr
                );
                write_descriptor_sets.push_back(write_descriptor_set);
            }
        }
        else
        {
            LOG_CORE_ERROR_TAG("Vulkan", "Shader layout set does not use image binding at #{}", binding_index);
        }
    }
}
} // portal
