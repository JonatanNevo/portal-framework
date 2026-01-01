//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include "portal/core/buffer.h"
#include "portal/engine/renderer/vulkan/vulkan_common.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/base/vulkan_resource.h"

namespace portal::renderer::vulkan::allocation
{
/**
 * Retrieves a reference to the VMA allocator singleton.  It will hold an opaque handle to the VMA
 * allocator between calls to `init` and `shutdown`.  Otherwise, it contains a null pointer.
 *
 * @return A reference to the VMA allocator singleton handle.
 */
VmaAllocator& get_vma_allocator();

/**
 * Initializes the VMA allocator with the specified device,
 * @param device The Vulkan device.
 */
void init(const vk::Instance& instance, const vk::PhysicalDevice& physical_device, const vk::Device& device);

/**
 * Shuts down the VMA allocator and releases all resources.  Should be preceded with a call to `init`.
 */
void shutdown();


/**
 * The `Allocated` class serves as a base class for wrappers around Vulkan that require memory allocation
 * (`VkImage` and `VkBuffer`).  This class mostly ensures proper behavior for a RAII pattern, preventing double-release by
 * preventing copy assignment and copy construction in favor of move semantics, as well as preventing default construction
 * in favor of explicit construction with a pre-existing handle or a populated create info struct.
 *
 * @note Constants used in this documentation in the form of `HOST_COHERENT` are shorthand for
 * `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` used for the sake of brevity.
 */
template <typename HandleType>
class Allocated : public VulkanResource<HandleType>
{
public:
    Allocated() = delete;
    Allocated(const Allocated&) = delete;
    Allocated(Allocated&& other) noexcept;
    Allocated& operator=(Allocated const& other) = delete;
    Allocated& operator=(Allocated&& other) = default;
    virtual Allocated& operator=(std::nullptr_t);

protected:
    /**
    * The VMA-specific constructor for new objects. This should only be visible to derived classes.
    * @param allocation_create_info All the non-resource-specific information needed by the VMA to allocate the memory.
    * @param args Additional constructor arguments needed for the derived class. Typically, a `vk::ImageCreateInfo` or `vk::BufferCreateInfo` struct.
    */
    template <typename... Args>
    explicit Allocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args);

    /**
     * This constructor is used when the handle is already created, and the user wants to wrap it in an `Allocated` object.
     * @note This constructor is used when the API provides us a pre-existing handle to something we didn't actually allocate, for instance
     * when we allocate a swap chain and access the images in it. In these cases the `allocation` member variable will remain null for the
     * lifetime of the wrapper object (which is NOT necessarily the lifetime of the handle) and the wrapper will make no attempt to apply
     * RAII semantics.
     */
    explicit Allocated(HandleType handle, vk::raii::Device* device_ = nullptr);

public:
    const HandleType* get() const;

    /**
     * Flushes memory if it is NOT `HOST_COHERENT` (which also implies `HOST_VISIBLE`).
     * This is a no-op for `HOST_COHERENT` memory.
     *
     * @param offset The offset into the memory to flush.  Defaults to 0.
     * @param size The size of the memory to flush.  Defaults to the entire block of memory.
     */
    void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize) const;

    /**
     * Retrieves a pointer to the host visible memory as an unsigned byte array.
     * @return The pointer to the host visible memory.
     * @note This performs no checking that the memory is actually mapped, so it's possible to get a nullptr
     */
    const void* get_data() const;

    /**
     * Retrieves a pointer to the host visible memory as an unsigned byte array.
     * @return The pointer to the host visible memory.
     * @note This performs no checking that the memory is actually mapped, so it's possible to get a nullptr
     */
    void* get_data();

    /**
     * Retrieves the raw Vulkan memory object.
     * @return The Vulkan memory object.
     */
    vk::DeviceMemory get_memory() const;

    /**
     * Maps Vulkan memory if it isn't already mapped to a host visible address. Does nothing if the
     * allocation is already mapped (including persistently mapped allocations).
     * @return Pointer to host visible memory.
     */
    virtual uint8_t* map();

    /**
     * Returns true if the memory is mapped (i.e. the object contains a pointer for the mapping).
     * This is true for both objects where `map` has been called as well as objects created with persistent
     * mapping, where no call to `map` is necessary.
     * @return mapping status.
     */
    bool mapped() const;

    /**
     * Unmaps Vulkan memory from the host visible address.  Does nothing if the memory is not mapped or
     * if the allocation is persistently mapped.
     */
    void unmap();

    /**
     * Copies the specified unsigned byte data into the mapped memory region.
     * @note no-op for non-persistently mapped memory,
     *
     * @param data The data to copy from.
     * @param size The amount of bytes to copy.
     * @param offset The offset to start the copying into the mapped data. Defaults to 0.
     */
    size_t update(const uint8_t* data, size_t size, size_t offset = 0) const;

    /**
     * Converts any non-byte data into bytes and then updates the buffer.  This allows the user to pass
     * arbitrary structure pointers to the update method, which will then be copied into the buffer as bytes.
     * @param data The data to copy from.
     * @param size The amount of bytes to copy.
     * @param offset The offset to start the copying into the mapped data. Defaults to 0.
     */
    size_t update(const void* data, size_t size, size_t offset = 0) const;

    /**
    * Copies a vector of items into the buffer.  This is a convenience method that allows the user to
    * pass a vector of items to the update method, which will then be copied into the buffer as bytes.
    *
    * This function DOES NOT automatically manage adhering to the alignment requirements of the items being copied,
    * for instance the `minUniformBufferOffsetAlignment` property of the [device](https://vulkan.gpuinfo.org/displaydevicelimit.php?name=minUniformBufferOffsetAlignment&platform=all).
    * If the data needs to be aligned on something other than `sizeof(T)`, the user must manage that themselves.
    * @param data The data vector to upload
    * @param offset The offset to start the copying into the mapped data
    * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
    */
    template <typename T>
    size_t update(const std::vector<T>& data, const size_t offset = 0)
    {
        return update(static_cast<const void*>(data.data()), data.size() * sizeof(T), offset);
    }

    /**
     * Another convenience method, similar to the vector update method, but for std::array. The same caveats apply.
     * @param data The data vector to upload
     * @param offset The offset to start the copying into the mapped data
     * @see update(std::vector<T> const &data, size_t offset = 0)
     * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
     */
    template <typename T, size_t N>
    size_t update(const std::array<T, N>& data, const size_t offset = 0)
    {
        return update(data.data(), data.size() * sizeof(T), offset);
    }

    /**
     * Updates the buffer with a `portal::Buffer` object.  This is a convenience method that allows the user to
     *
     * @param buffer The buffer to copy from.
     * @param offset The offset to start the copying into the mapped data. Defaults to 0.
     * @return The number of bytes copied.
     */
    size_t update(const portal::Buffer& buffer, const size_t offset = 0) const;

    /**
     * Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
     * pass an object to the update method, which will then be copied into the buffer as bytes.  The name difference
     * is to avoid ambiguity with the `update` method signatures (including the non-templated version)
     * @param object The object to convert into byte data
     * @param offset The offset to start the copying into the mapped data
     * @deprecated Use the `updateTyped` method that uses the `vk::ArrayProxy` class instead.
     */
    template <class T>
    size_t convert_and_update(const T& object, const size_t offset = 0)
    {
        return update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
    }

    /**
     * Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
     * pass an object to the update method, which will then be copied into the buffer as bytes.  The use of the `vk::ArrayProxy`
     * type here to wrap the passed data means you can use any type related to T that can be used as a constructor to `vk::ArrayProxy`.
     * This includes `T`, `std::vector<T>`, `std::array<T, N>`, and `vk::ArrayProxy<T>`.
     *
     * @remark This was previously not feasible as it would have been undesirable to create a strong coupling with the
     * C++ Vulkan bindings where the `vk::ArrayProxy` type is defined.  However, structural changes have ensured that this
     * coupling is always present, so the `vk::ArrayProxy` may as well be used to our advantage here.
     *
     * @note This function DOES NOT automatically manage adhering to the alignment requirements of the items being copied,
     * for instance the `minUniformBufferOffsetAlignment` property of the [device](https://vulkan.gpuinfo.org/displaydevicelimit.php?name=minUniformBufferOffsetAlignment&platform=all).
     * If the data needs to be aligned on something other than `sizeof(T)`, the user must manage that themselves.
     *
     * @todo create `updateTypedAligned` which has an additional argument specifying the required GPU alignment of the elements of the array.
     */
    template <class T>
    size_t update_typed(const vk::ArrayProxy<T>& object, const size_t offset = 0)
    {
        return update(reinterpret_cast<const uint8_t*>(object.data()), object.size() * sizeof(T), offset);
    }

protected:
    /**
     * Internal method to actually create the buffer, allocate the memory and bind them.
     * Should only be called from the `Buffer` derived class.
     *
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    [[nodiscard]] vk::Buffer create_buffer(const vk::BufferCreateInfo& create_info);

    /**
     * Internal method to actually create the image, allocate the memory and bind them.
     * Should only be called from the `Image` derived class.
     *
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    [[nodiscard]] vk::Image create_image(const vk::ImageCreateInfo& create_info);

    /**
     * The post_create method is called after the creation of a buffer or image to store the allocation info internally.  Derived classes
     * could in theory override this to ensure any post-allocation operations are performed, but the base class should always be called to ensure
     * the allocation info is stored.
     * Should only be called in the corresponding `create_xxx` methods.
     */
    virtual void post_create(const VmaAllocationInfo& allocation_info);

    /**
     * Internal method to actually destroy the buffer and release the allocated memory.  Should
     * only be called from the `Buffer` derived class.
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    void destroy_buffer(vk::Buffer buffer);

    /**
     * Internal method to actually destroy the image and release the allocated memory.  Should
     * only be called from the `Image` derived class.
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    void destroy_image(vk::Image image);

    /**
     * Clears the internal state.  Can be overridden by derived classes to perform additional cleanup of members.
     * Should only be called in the corresponding `destroy_xxx` methods.
     */
    void clear();

private:
    VmaAllocationCreateInfo allocation_create_info = {};
    VmaAllocation allocation = nullptr;

    /**
     * A pointer to the allocation memory, if the memory is HOST_VISIBLE and is currently (or persistently) mapped.
     * Contains null otherwise.
     */
    uint8_t* mapped_data = nullptr;

    /**
     * This flag is set to true if the memory is coherent and doesn't need to be flushed after writes.
     *
     * @note This is initialized at allocation time to avoid subsequent need to call a function to fetch the
     * allocation information from the VMA, since this property won't change for the lifetime of the allocation.
     */
    bool coherent = false;

    /**
     * This flag is set to true if the memory is persistently mapped (i.e. not just HOST_VISIBLE, but available
     * as a pointer to the application for the lifetime of the allocation).
     *
     * @note This is initialized at allocation time to avoid subsequent need to call a function to fetch the
     * allocation information from the VMA, since this property won't change for the lifetime of the allocation.
     */
    bool persistent = false;
};

template <typename HandleType>
Allocated<HandleType>::Allocated(Allocated&& other) noexcept
    : VulkanResource<HandleType>(static_cast<VulkanResource<HandleType>&&>(other)),
      allocation_create_info(std::exchange(other.allocation_create_info, {})),
      allocation(std::exchange(other.allocation, {})),
      mapped_data(std::exchange(other.mapped_data, {})),
      coherent(std::exchange(other.coherent, {})),
      persistent(std::exchange(other.persistent, {}))
{}

template <typename HandleType>
Allocated<HandleType>& Allocated<HandleType>::operator=(std::nullptr_t)
{
    allocation_create_info = {};
    allocation = nullptr;
    mapped_data = nullptr;
    coherent = false;
    persistent = false;
    return *static_cast<Allocated*>(this);
}

template <typename HandleType>
template <typename... Args>
Allocated<HandleType>::Allocated(const VmaAllocationCreateInfo& allocation_create_info, Args&&... args) :
    VulkanResource<HandleType>(std::forward<Args>(args)...),
    allocation_create_info(allocation_create_info) {}

template <typename HandleType>
Allocated<HandleType>::Allocated(HandleType handle, vk::raii::Device* device_) : VulkanResource<HandleType>(handle, device_) {}

template <typename HandleType>
const HandleType* Allocated<HandleType>::get() const
{
    return &VulkanResource<HandleType>::get_handle();
}

template <typename HandleType>
void Allocated<HandleType>::flush(const vk::DeviceSize offset, const vk::DeviceSize size) const
{
    if (!coherent)
        vmaFlushAllocation(get_vma_allocator(), allocation, offset, size);
}

template <typename HandleType>
const void* Allocated<HandleType>::get_data() const
{
    return mapped_data;
}

template <typename HandleType>
void* Allocated<HandleType>::get_data()
{
    return mapped_data;
}

template <typename HandleType>
vk::DeviceMemory Allocated<HandleType>::get_memory() const
{
    VmaAllocationInfo allocation_info = {};
    vmaGetAllocationInfo(get_vma_allocator(), allocation, &allocation_info);
    return allocation_info.deviceMemory;
}

template <typename HandleType>
uint8_t* Allocated<HandleType>::map()
{
    if (!persistent && !mapped())
    {
        [[maybe_unused]] const auto result = vmaMapMemory(get_vma_allocator(), allocation, reinterpret_cast<void**>(&mapped_data));
        PORTAL_ASSERT(result == VK_SUCCESS, "Failed to map memory");
    }
    else
        LOG_WARN_TAG("Vulkan", "Attempting to map a persistent or mapped memory");
    return mapped_data;
}

template <typename HandleType>
bool Allocated<HandleType>::mapped() const
{
    return mapped_data != nullptr;
}

template <typename HandleType>
void Allocated<HandleType>::unmap()
{
    if (!persistent && mapped())
    {
        vmaUnmapMemory(get_vma_allocator(), allocation);
        mapped_data = nullptr;
    }
}

template <typename HandleType>
size_t Allocated<HandleType>::update(const uint8_t* data, const size_t size, const size_t offset) const
{
    if (persistent)
    {
        std::copy_n(data, size, mapped_data + offset);
        flush();
    }
    else
    {
        LOG_WARN_TAG("Vulkan", "Attempting to update a persistent memory");
    }
    return size;
}

template <typename HandleType>
size_t Allocated<HandleType>::update(const void* data, const size_t size, const size_t offset) const
{
    return update(static_cast<const uint8_t*>(data), size, offset);
}

template <typename HandleType>
size_t Allocated<HandleType>::update(const portal::Buffer& buffer, const size_t offset) const
{
    return update(buffer.data, buffer.size, offset);
}

template <typename HandleType>
void Allocated<HandleType>::clear()
{
    mapped_data = nullptr;
    persistent = false;
    allocation_create_info = VmaAllocationCreateInfo{};
}

template <typename HandleType>
vk::Buffer Allocated<HandleType>::create_buffer(const vk::BufferCreateInfo& create_info)
{
    VmaAllocationInfo allocation_info{};
    VkBuffer buffer_handle = nullptr;
    VmaAllocation new_allocation = nullptr;
    const auto result = vmaCreateBuffer(
        get_vma_allocator(),
        reinterpret_cast<const VkBufferCreateInfo*>(&create_info),
        &allocation_create_info,
        &buffer_handle,
        &new_allocation,
        &allocation_info
    );

    const auto buffer = vk::Buffer(buffer_handle);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer");
    }
    this->allocation = new_allocation;
    post_create(allocation_info);
    return buffer;
}

template <typename HandleType>
vk::Image Allocated<HandleType>::create_image(const vk::ImageCreateInfo& create_info)
{
    PORTAL_ASSERT(0 < create_info.mipLevels, "Image must have at least one mip level");
    PORTAL_ASSERT(0 < create_info.arrayLayers, "Image must have at least one array layer");
    PORTAL_ASSERT(create_info.usage, "Image must have at least one usage type");

    VmaAllocationInfo allocation_info{};
    VkImage image_handle;
    VmaAllocation new_allocation = nullptr;
    const auto result = vmaCreateImage(
        get_vma_allocator(),
        reinterpret_cast<const VkImageCreateInfo*>(&create_info),
        &allocation_create_info,
        &image_handle,
        &new_allocation,
        &allocation_info
    );

    const auto image = vk::Image(image_handle);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image");
    }
    this->allocation = new_allocation;
    post_create(allocation_info);
    return image;
}

template <typename HandleType>
void Allocated<HandleType>::post_create(const VmaAllocationInfo& allocation_info)
{
    VkMemoryPropertyFlags memory_properties_raw{};
    vmaGetAllocationMemoryProperties(get_vma_allocator(), allocation, &memory_properties_raw);

    const auto memory_properties = vk::MemoryPropertyFlags(memory_properties_raw);
    coherent = (memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent) == vk::MemoryPropertyFlagBits::eHostCoherent;
    mapped_data = static_cast<uint8_t*>(allocation_info.pMappedData);
    persistent = mapped();
}

template <typename HandleType>
void Allocated<HandleType>::destroy_buffer(const vk::Buffer buffer)
{
    if (buffer != nullptr && allocation)
    {
        unmap();
        vmaDestroyBuffer(get_vma_allocator(), buffer, this->allocation);
        clear();
    }
}

template <typename HandleType>
void Allocated<HandleType>::destroy_image(const vk::Image image)
{
    if (image != nullptr && allocation)
    {
        unmap();
        vmaDestroyImage(get_vma_allocator(), image, this->allocation);
        clear();
    }
}
} // portal
