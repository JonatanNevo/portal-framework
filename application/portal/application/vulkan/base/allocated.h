//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>

#include "vulkan_resource.h"


namespace portal::vulkan
{
class Device;
}

namespace portal::vulkan::allocated
{
/**
 * @brief Retrieves a reference to the VMA allocator singleton.  It will hold an opaque handle to the VMA
 * allocator between calls to `init` and `shutdown`.  Otherwise it contains a null pointer.
 * @return A reference to the VMA allocator singleton handle.
 */
vma::Allocator& get_memory_allocator();

/**
 * @brief Initializes the VMA allocator with the specified device,
 * @param device The Vulkan device.
 */
void init(const Device& device);

/**
 * @brief Shuts down the VMA allocator and releases all resources.  Should be preceeded with a call to `init`.
 */
void shutdown();

/**
 * @brief The `Allocated` class serves as a base class for wrappers around Vulkan that require memory allocation
 * (`VkImage` and `VkBuffer`).  This class mostly ensures proper behavior for a RAII pattern, preventing double-release by
 * preventing copy assignment and copy construction in favor of move semantics, as well as preventing default construction
 * in favor of explicit construction with a pre-existing handle or a populated create info struct.
 *
 * This project uses the [VMA](https://gpuopen.com/vulkan-memory-allocator/) to handle the low
 * level details of memory allocation and management, as it hides away many of the messyy details of
 * memory allocation when a user is first learning Vulkan, but still allows for fine grained control
 * when a user becomes more experienced and the situation calls for it.
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

protected:
    /**
     * @brief The VMA-specific constructor for new objects. This should only be visible to derived classes.
     * @param allocation_create_info All of the non-resource-specific information needed by the VMA to allocate the memory.
     * @param args Additional constructor arguments needed for the derived class. Typically a `VkImageCreateInfo` or `VkBufferCreateInfo` struct.
     */
    template <typename... Args>
    explicit Allocated(const vma::AllocationCreateInfo& allocation_create_info, Args&&... args);

    /**
     * @brief This constructor is used when the handle is already created, and the user wants to wrap it in an `Allocated` object.
     * @note This constructor is used when the API provides us a pre-existing handle to something we didn't actually allocate, for instance
     * when we allocate a swapchain and access the images in it.  In these cases the `allocation` member variable will remain null for the
     * lifetime of the wrapper object (which is NOT necessarily the lifetime of the handle) and the wrapper will make no attempt to apply
     * RAII semantics.
     */
    explicit Allocated(HandleType handle, Device* device_ = nullptr);

public:
    const HandleType* get() const;

    /**
     * @brief Flushes memory if it is NOT `HOST_COHERENT` (which also implies `HOST_VISIBLE`).
     * This is a no-op for `HOST_COHERENT` memory.
     *
     * @param offset The offset into the memory to flush.  Defaults to 0.
     * @param size The size of the memory to flush.  Defaults to the entire block of memory.
     */
    void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) const;

    /**
     * @brief Retrieves a pointer to the host visible memory as an unsigned byte array.
     * @return The pointer to the host visible memory.
     * @note This performs no checking that the memory is actually mapped, so it's possible to get a nullptr
     */
    const uint8_t* get_data() const;

    /**
     * @brief Retrieves the raw Vulkan memory object.
     * @return The Vulkan memory object.
     */
    vk::DeviceMemory get_memory() const;

    /**
     * @brief Maps Vulkan memory if it isn't already mapped to a host visible address. Does nothing if the
     * allocation is already mapped (including persistently mapped allocations).
     * @return Pointer to host visible memory.
     */
    virtual uint8_t* map();

    /**
     * @brief Returns true if the memory is mapped (i.e. the object contains a pointer for the mapping).
     * This is true for both objects where `map` has been called as well as objects created with persistent
     * mapping, where no call to `map` is necessary.
     * @return mapping status.
     */
    bool mapped() const;

    /**
     * @brief Unmaps Vulkan memory from the host visible address.  Does nothing if the memory is not mapped or
     * if the allocation is persistently mapped.
     */
    void unmap();

    /**
     * @brief Copies the specified unsigned byte data into the mapped memory region.
     * @note no-op for non-persistently mapped memory,
     *
     * @param data The data to copy from.
     * @param size The amount of bytes to copy.
     * @param offset The offset to start the copying into the mapped data. Defaults to 0.
     */
    size_t update(const uint8_t* data, size_t size, size_t offset = 0) const;

    /**
     * @brief Converts any non-byte data into bytes and then updates the buffer.  This allows the user to pass
     * arbitrary structure pointers to the update method, which will then be copied into the buffer as bytes.
     * @param data The data to copy from.
     * @param size The amount of bytes to copy.
     * @param offset The offset to start the copying into the mapped data. Defaults to 0.
     */
    size_t update(const void* data, size_t size, size_t offset = 0) const;

    /**
     * @brief Copies a vector of items into the buffer.  This is a convenience method that allows the user to
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
     * @brief Another convenience method, similar to the vector update method, but for std::array. The same caveats apply.
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
     * @brief Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
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
     * @brief Copies an object as byte data into the buffer.  This is a convenience method that allows the user to
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
     * @brief Internal method to actually create the buffer, allocate the memory and bind them.
     * Should only be called from the `Buffer` derived class.
     *
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    [[nodiscard]] vk::Buffer create_buffer(const vk::BufferCreateInfo& create_info);

    /**
     * @brief Internal method to actually create the image, allocate the memory and bind them.
     * Should only be called from the `Image` derived class.
     *
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */
    [[nodiscard]] vk::Image create_image(const vk::ImageCreateInfo& create_info);

    /**
     * @brief The post_create method is called after the creation of a buffer or image to store the allocation info internally.  Derived classes
     * could in theory override this to ensure any post-allocation operations are performed, but the base class should always be called to ensure
     * the allocation info is stored.
     * Should only be called in the corresponding `create_xxx` methods.
     */
    virtual void post_create(vma::AllocationInfo const& allocation_info);

    /**
     * @brief Internal method to actually destroy the buffer and release the allocated memory.  Should
     * only be called from the `Buffer` derived class.
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */

    void destroy_buffer(vk::Buffer buffer);
    /**
     * @brief Internal method to actually destroy the image and release the allocated memory.  Should
     * only be called from the `Image` derived class.
     * Present in this common base class in order to allow the internal state members to remain `private`
     * instead of `protected`, and because it (mostly) isolates interaction with the VMA to a single class
     */

    void destroy_image(vk::Image image);

    /**
     * @brief Clears the internal state.  Can be overridden by derived classes to perform additional cleanup of members.
     * Should only be called in the corresponding `destroy_xxx` methods.
     */
    void clear();

private:
    vma::AllocationCreateInfo allocation_create_info = {};
    vma::Allocation allocation = nullptr;

    /**
     * @brief A pointer to the allocation memory, if the memory is HOST_VISIBLE and is currently (or persistently) mapped.
     * Contains null otherwise.
     */
    uint8_t* mapped_data = nullptr;

    /**
     * @brief This flag is set to true if the memory is coherent and doesn't need to be flushed after writes.
     *
     * @note This is initialized at allocation time to avoid subsequent need to call a function to fetch the
     * allocation information from the VMA, since this property won't change for the lifetime of the allocation.
     */
    bool coherent = false;

    /**
     * @brief This flag is set to true if the memory is persistently mapped (i.e. not just HOST_VISIBLE, but available
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
template <typename... Args>
Allocated<HandleType>::Allocated(const vma::AllocationCreateInfo& allocation_create_info, Args&&... args):
    VulkanResource<HandleType>(std::forward<Args>(args)...),
    allocation_create_info(allocation_create_info) {}

template <typename HandleType>
Allocated<HandleType>::Allocated(HandleType handle, Device* device_): VulkanResource<HandleType>(handle, device_) {}

template <typename HandleType>
const HandleType* Allocated<HandleType>::get() const
{
    return &VulkanResource<HandleType>::get_handle();
}

template <typename HandleType>
void Allocated<HandleType>::flush(vk::DeviceSize offset, vk::DeviceSize size) const
{
    if (!coherent)
        get_memory_allocator().flushAllocation(allocation, offset, size);
}

template <typename HandleType>
const uint8_t* Allocated<HandleType>::get_data() const
{
    return mapped_data;
}

template <typename HandleType>
vk::DeviceMemory Allocated<HandleType>::get_memory() const
{
    const auto alloc_info = get_memory_allocator().getAllocationInfo(allocation);
    return alloc_info.deviceMemory;
}

template <typename HandleType>
uint8_t* Allocated<HandleType>::map()
{
    if (!persistent && !mapped())
    {
        mapped_data = static_cast<uint8_t*>(get_memory_allocator().mapMemory(allocation));
        PORTAL_CORE_ASSERT(mapped_data, "Failed to map memory");
    }
    else
        LOG_CORE_WARN_TAG("Vulkan", "Attempting to map a persistent or mapped memory");
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
        get_memory_allocator().unmapMemory(allocation);
        mapped_data = nullptr;
    }
    else
        LOG_CORE_WARN_TAG("Vulkan", "Attempting to unmap a persistent or unmapped memory");
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
        LOG_CORE_WARN_TAG("Vulkan", "Attempting to update a persistent memory");
    }
    return size;
}

template <typename HandleType>
size_t Allocated<HandleType>::update(const void* data, size_t size, size_t offset) const
{
    return update(static_cast<const uint8_t*>(data), size, offset);
}

template <typename HandleType>
void Allocated<HandleType>::clear()
{
    mapped_data = nullptr;
    persistent = false;
    allocation_create_info = vma::AllocationCreateInfo{};
}

template <typename HandleType>
vk::Buffer Allocated<HandleType>::create_buffer(const vk::BufferCreateInfo& create_info)
{
    vma::AllocationInfo allocation_info{};
    const auto [buffer, allocation] = get_memory_allocator().createBuffer(
        create_info,
        allocation_create_info,
        allocation_info
    );
    if (buffer == nullptr)
    {
        throw std::runtime_error("failed to create buffer");
    }
    this->allocation = allocation;
    post_create(allocation_info);
    return buffer;
}

template <typename HandleType>
vk::Image Allocated<HandleType>::create_image(const vk::ImageCreateInfo& create_info)
{
    PORTAL_CORE_ASSERT(0 < create_info.mipLevels, "Image must have at least one mip level");
    PORTAL_CORE_ASSERT(0 < create_info.arrayLayers, "Image must have at least one array layer");
    PORTAL_CORE_ASSERT(create_info.usage, "Image must have at least one usage type");

    vma::AllocationInfo allocation_info{};
    const auto [image, allocation] = get_memory_allocator().createImage(
        create_info,
        allocation_create_info,
        allocation_info
    );
    if (image == nullptr)
    {
        throw std::runtime_error("failed to create image");
    }
    this->allocation = allocation;
    post_create(allocation_info);
    return image;
}

template <typename HandleType>
void Allocated<HandleType>::post_create(vma::AllocationInfo const& allocation_info)
{
    const auto memory_properties = get_memory_allocator().getAllocationMemoryProperties(allocation);
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
        get_memory_allocator().destroyBuffer(buffer, allocation);
        clear();
    }
}

template <typename HandleType>
void Allocated<HandleType>::destroy_image(const vk::Image image)
{
    if (image != nullptr && allocation)
    {
        unmap();
        get_memory_allocator().destroyImage(image, allocation);
        clear();
    }
}
}

