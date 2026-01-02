//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/core/debug/assert.h"


namespace portal::renderer::vulkan
{
class VulkanDevice;

/**
 * @class VulkanResource
 * @brief CRTP base class for all Vulkan resource wrappers
 *
 * Provides handle storage, device back-references, debug naming, and move-only semantics.
 * Uses CRTP to avoid virtual function overhead while providing common functionality.
 *
 * @tparam Handle The Vulkan handle type (vk::Buffer, vk::Image, vk::Pipeline, etc.)
 */
template <typename Handle>
class VulkanResource
{
public:
    /**
     * @brief Constructs a Vulkan resource wrapper
     * @param handle The Vulkan handle to wrap
     * @param device Pointer to the creating device
     */
    explicit VulkanResource(Handle handle, const VulkanDevice* device) : device(device), handle(handle) {}
    VulkanResource(const VulkanResource&) = delete;
    VulkanResource& operator=(const VulkanResource&) = delete;

    VulkanResource(VulkanResource&& other) noexcept : debug_name(std::exchange(other.debug_name, {})),
                                                      device(other.device),
                                                      handle(std::exchange(other.handle, {})) {}

    VulkanResource& operator=(VulkanResource&& other) noexcept
    {
        debug_name = std::exchange(other.debug_name, {});
        handle = std::exchange(other.handle, {});
        return *this;
    }

    virtual ~VulkanResource() = default;

    /** @brief Gets the debug name assigned to this resource */
    [[nodiscard]] const std::string& get_debug_name() const { return debug_name; }

    /**
     * @brief Gets the device that created this resource
     * @return Reference to the VulkanDevice
     */
    [[nodiscard]] const VulkanDevice& get_device() const
    {
        PORTAL_ASSERT(device != nullptr, "Device is nullptr");
        return *device;
    }

    /** @brief Gets mutable reference to the Vulkan handle */
    Handle& get_handle() { return handle; }

    /** @brief Gets const reference to the Vulkan handle */
    const Handle& get_handle() const { return handle; }

    /**
     * @brief Converts handle to uint64_t for debug APIs
     * @return Handle as uint64_t (handles 32-bit non-dispatchable handles on 32-bit platforms)
     */
    [[nodiscard]] uint64_t get_handle_u64() const
    {
        // See https://github.com/KhronosGroup/Vulkan-Docs/issues/368.
        // Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
        // Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
        if constexpr (sizeof(Handle) == sizeof(uint32_t))
        {
            return static_cast<uint64_t>(std::bit_cast<uint32_t>(handle));
        }
        else
        {
            return std::bit_cast<uint64_t>(handle);
        }
    }

    /** @brief Gets the Vulkan object type from the handle */
    [[nodiscard]] vk::ObjectType get_object_type() const { return Handle::objectType; }

    /** @brief Checks if device pointer is valid */
    [[nodiscard]] bool has_device() const { return device != nullptr; }

    /** @brief Checks if handle is valid (non-null) */
    [[nodiscard]] bool has_handle() const { return handle != nullptr; }

    /**
     * @brief Sets the Vulkan handle
     * @param hdl The new handle value
     */
    void set_handle(Handle hdl) { handle = hdl; }

    /**
     * @brief Sets debug name and propagates to GPU debuggers
     * @param name The debug name (appears in RenderDoc, NSight, etc.)
     */
    void set_debug_name(const std::string& name)
    {
        debug_name = name;
        if (!debug_name.empty())
            get_device().set_debug_name(get_object_type(), get_handle_u64(), debug_name.c_str());
    }

protected:
    std::string debug_name;

private:
    const VulkanDevice* device;
    Handle handle;
};
}
