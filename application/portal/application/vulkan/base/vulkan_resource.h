//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once

#include <vulkan/vulkan.hpp>

#include "portal/core/assert.h"

namespace portal::vulkan
{
class Device;

/// Inherit this for any Vulkan object with a handle of type `Handle`.
///
/// This allows the derived class to store a Vulkan handle, and also a pointer to the parent portal::vulkan::Device.
/// It also allows to set a debug name for any Vulkan object.
template <typename Handle>
class VulkanResource
{
public:
    using DeviceType = Device;
    using ObjectType = vk::ObjectType;

    explicit VulkanResource(Handle handle = nullptr, DeviceType* device = nullptr) : device(device), handle(handle) {}


    VulkanResource(const VulkanResource&) = delete;
    VulkanResource& operator=(const VulkanResource&) = delete;

    VulkanResource(VulkanResource&& other) noexcept: debug_name(std::exchange(other.debug_name, {})),
                                                     device(std::exchange(other.device, {})),
                                                     handle(std::exchange(other.handle, {})) {}

    VulkanResource& operator=(VulkanResource&& other) noexcept
    {
        debug_name = std::exchange(other.debug_name, {});
        device = std::exchange(other.device, {});
        handle = std::exchange(other.handle, {});
        return *this;
    }

    virtual ~VulkanResource() = default;

    [[nodiscard]] const std::string& get_debug_name() const { return debug_name; }

    DeviceType& get_device()
    {
        PORTAL_CORE_ASSERT(device != nullptr, "Device is nullptr");
        return *device;
    }

    [[nodiscard]] const DeviceType& get_device() const
    {
        PORTAL_CORE_ASSERT(device != nullptr, "Device is nullptr");
        return *device;
    }

    [[nodiscard]] vk::Device get_device_handle() const { return get_device().get_handle(); }

    Handle& get_handle() { return handle; }
    const Handle& get_handle() const { return handle; }

    [[nodiscard]] uint64_t get_handle_u64() const
    {
        // See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
        // Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
        // Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
        using UintHandle = std::conditional_t<sizeof(Handle) == sizeof(uint32_t), uint32_t, uint64_t>;

        return static_cast<uint64_t>(*reinterpret_cast<UintHandle const*>(&handle));
    }

    [[nodiscard]] ObjectType get_object_type() const { return Handle::objectType; }
    [[nodiscard]] bool has_device() const { return device != nullptr; }
    [[nodiscard]] bool has_handle() const { return handle != nullptr; }

    void set_debug_name(const std::string& name);

    void set_handle(Handle hdl) { handle = hdl; }

private:
    std::string debug_name;
    Device* device;
    Handle handle;
};

template <typename Handle>
void VulkanResource<Handle>::set_debug_name(const std::string& name) {
    debug_name = name;

    if (device && !debug_name.empty())
        get_device().get_debug_utils().set_debug_name(get_device().get_handle(), get_object_type(), get_handle_u64(), debug_name.c_str());
}

}
