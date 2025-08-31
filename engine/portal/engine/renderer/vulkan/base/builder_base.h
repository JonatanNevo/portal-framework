//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace portal::renderer::vulkan
{
/**
 * Many Vulkan resource types (most notably Images and to a lesser extent Buffers)
 * and their corresponding memory allocations have many parameters that need to be setup
 * when creating them.  Although many of these have reasonable defaults, constructors with
 * numerous arguments, some or all of which may have default arguments, aren't well suited
 * to partial customization.  This is a common failing of languages that don't support named
 * arguments and has led to the common use of the [builder pattern](https://en.wikipedia.org/wiki/Builder_pattern),
 * where a helper class is used to store all the options that can be tweaked for an object
 * when it's created. A builder class will have reasonable defaults where appropriate and only
 * require arguments for the builder constructor when a value is always required for creation to occur
 * (for example, the size of a buffer or the extent of an image).  Remaining parameters can be set
 * with methods on the builder class, which return a reference to the builder object, allowing
 * chaining of the method calls.
 *
 * This builder class serves as a base containing options that are common to all
 * [VMA](https://gpuopen.com/vulkan-memory-allocator/) allocated and managed resources.
 * For instance, the VMA create and usage flags are set here, but the image or buffer
 * usage flags are handled in the derived builder classes specific to those types.
 *
 * The following is an example of how the builder pattern is used in the codebase:
 ```cpp
        vkb::core::ImageBuilder(VkExtent3D{grid_width, grid_height, 1})
            .with_format(VK_FORMAT_R8G8B8A8_UNORM)
            .with_usage(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
            .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
            .with_sample_count(VK_SAMPLE_COUNT_1_BIT)
            .with_mip_levels(1)
            .with_array_layers(1)
            .with_tiling(VK_IMAGE_TILING_OPTIMAL)
            .with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data())
            .with_sharing_mode(sharing_mode));
 ```
 * The actual image can be created with `build()` which returns a `vkb::core::Image` or `buildPtr` which returns a `std::unique_ptr<vkb::core::Image>`.
 * Alternatively, the builder can be used as an argument to the `Image` constructor, which will build the image for you in place.
 * @note The builder pattern is intended to displace the currently used `vkb::core::Image` and `vkb::core::Buffer` constructors with numerous
 * arguments, but this is a work in progress and not currently in wide use in the codebase.
 *
 * @tparam BuilderType Allow the same builder base class to be used
 * with a variety of subclasses while using casting to return the correct dervied type
 * from the modifier methods.
 * @tparam CreateInfoType The type of the Vulkan create info structure. Either a `VkSomethingCreateInfo`
 * or `vk::SomethingCreateInfo` for the C or C++ API respectively.
 */

template <typename BuilderType, typename CreateInfoType>
struct BuilderBase
{
public:
    virtual ~BuilderBase() = default;

    [[nodiscard]] const VmaAllocationCreateInfo& get_allocation_create_info() const
    {
        return alloc_create_info;
    }
    const CreateInfoType& get_create_info() const
    {
        return create_info;
    }

    [[nodiscard]] const std::string& get_debug_name() const
    {
        return debug_name;
    }

    BuilderType& with_debug_name(const std::string& name)
    {
        debug_name = name;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_implicit_sharing_mode()
    {
        create_info.sharingMode = (1 < create_info.queueFamilyIndexCount) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_memory_type_bits(uint32_t type_bits)
    {
        alloc_create_info.memoryTypeBits = type_bits;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_queue_families(uint32_t count, const uint32_t* family_indices)
    {
        create_info.queueFamilyIndexCount = count;
        create_info.pQueueFamilyIndices = family_indices;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_queue_families(const std::vector<uint32_t>& queue_families)
    {
        return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
    }

    BuilderType& with_sharing_mode(vk::SharingMode sharing_mode)
    {
        create_info.sharingMode = sharing_mode;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_vma_flags(const VmaAllocationCreateFlags flags)
    {
        alloc_create_info.flags = flags;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_vma_pool(const VmaPool pool)
    {
        alloc_create_info.pool = pool;
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_vma_preferred_flags(const vk::MemoryPropertyFlags flags)
    {
        alloc_create_info.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_vma_required_flags(const vk::MemoryPropertyFlags flags)
    {
        alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
        return static_cast<BuilderType&>(*this);
    }

    BuilderType& with_vma_usage(VmaMemoryUsage usage)
    {
        alloc_create_info.usage = usage;
        return static_cast<BuilderType&>(*this);
    }

    CreateInfoType& get_create_info()
    {
        return create_info;
    }

protected:
    explicit BuilderBase(const CreateInfoType& create_info): create_info(create_info)
    {
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    }


protected:
    VmaAllocationCreateInfo alloc_create_info = {};
    CreateInfoType create_info = {};
    std::string debug_name = {};
};

}
