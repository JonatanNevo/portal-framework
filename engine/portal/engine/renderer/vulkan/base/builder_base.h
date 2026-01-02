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
 * @class BuilderBase
 * @brief Base class for resource builders, provides common VMA allocation options
 *
 * Contains VMA allocation configuration methods shared by all resource builders (BufferBuilder,
 * ImageBuilder, etc.). Derived builders add resource-specific options like buffer usage flags
 * or image formats. Methods return the derived type for method chaining.
 *
 * Usage:
 * @code
 * auto buffer = BufferBuilder(1024)
 *     .with_usage(vk::BufferUsageFlagBits::eStorageBuffer)
 *     .with_vma_usage(VMA_MEMORY_USAGE_GPU_ONLY)
 *     .build(device);
 * @endcode
 *
 * @tparam BuilderType The derived builder type (for CRTP method chaining)
 * @tparam CreateInfoType The Vulkan create info structure (vk::BufferCreateInfo, vk::ImageCreateInfo, etc.)
 */

template <typename BuilderType, typename CreateInfoType>
struct BuilderBase
{
public:
    virtual ~BuilderBase() = default;

    /** @brief Gets the VMA allocation create info */
    [[nodiscard]] const VmaAllocationCreateInfo& get_allocation_create_info() const
    {
        return alloc_create_info;
    }

    /** @brief Gets the Vulkan create info structure */
    const CreateInfoType& get_create_info() const
    {
        return create_info;
    }

    /** @brief Gets the debug name */
    [[nodiscard]] const std::string& get_debug_name() const
    {
        return debug_name;
    }

    /**
     * @brief Sets debug name for GPU debuggers
     * @param name Debug name (appears in RenderDoc, NSight, etc.)
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_debug_name(const std::string& name)
    {
        debug_name = name;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Automatically sets sharing mode based on queue family count
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_implicit_sharing_mode()
    {
        create_info.sharingMode = (1 < create_info.queueFamilyIndexCount) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets memory type bits for VMA allocation
     * @param type_bits Bitfield of allowed memory types
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_memory_type_bits(uint32_t type_bits)
    {
        alloc_create_info.memoryTypeBits = type_bits;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets queue families that can access this resource
     * @param count Number of queue family indices
     * @param family_indices Array of queue family indices
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_queue_families(uint32_t count, const uint32_t* family_indices)
    {
        create_info.queueFamilyIndexCount = count;
        create_info.pQueueFamilyIndices = family_indices;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets queue families that can access this resource
     * @param queue_families Vector of queue family indices
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_queue_families(const std::vector<uint32_t>& queue_families)
    {
        return with_queue_families(static_cast<uint32_t>(queue_families.size()), queue_families.data());
    }

    /**
     * @brief Sets queue sharing mode (Exclusive or Concurrent)
     * @param sharing_mode The sharing mode
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_sharing_mode(vk::SharingMode sharing_mode)
    {
        create_info.sharingMode = sharing_mode;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets VMA allocation flags (e.g., VMA_ALLOCATION_CREATE_MAPPED_BIT)
     * @param flags VMA allocation create flags
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_vma_flags(const VmaAllocationCreateFlags flags)
    {
        alloc_create_info.flags = flags;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets VMA memory pool to allocate from
     * @param pool The VMA pool
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_vma_pool(const VmaPool pool)
    {
        alloc_create_info.pool = pool;
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets preferred memory property flags (e.g., HOST_CACHED)
     * @param flags Preferred memory properties
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_vma_preferred_flags(const vk::MemoryPropertyFlags flags)
    {
        alloc_create_info.preferredFlags = static_cast<VkMemoryPropertyFlags>(flags);
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets required memory property flags (e.g., HOST_VISIBLE | HOST_COHERENT)
     * @param flags Required memory properties
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_vma_required_flags(const vk::MemoryPropertyFlags flags)
    {
        alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(flags);
        return static_cast<BuilderType&>(*this);
    }

    /**
     * @brief Sets VMA memory usage hint (GPU_ONLY, CPU_TO_GPU, etc.)
     * @param usage VMA memory usage
     * @return Reference to derived builder for chaining
     */
    BuilderType& with_vma_usage(VmaMemoryUsage usage)
    {
        alloc_create_info.usage = usage;
        return static_cast<BuilderType&>(*this);
    }

    /** @brief Gets mutable reference to Vulkan create info */
    CreateInfoType& get_create_info()
    {
        return create_info;
    }

protected:
    explicit BuilderBase(const CreateInfoType& create_info) : create_info(create_info)
    {
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    }

protected:
    VmaAllocationCreateInfo alloc_create_info = {};
    CreateInfoType create_info = {};
    std::string debug_name = {};
};
}
