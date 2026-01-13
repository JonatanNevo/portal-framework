//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "device/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/allocated_buffer.h"
#include "portal/engine/renderer/vulkan/allocated_image.h"
#include "portal/engine/renderer/vulkan/vulkan_common.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/renderer/device/device.h"
#include "portal/engine/renderer/vulkan/queue/vulkan_queue.h"

namespace portal::renderer::vulkan
{
class DescriptorLayoutBuilder;
}

namespace portal::renderer::vulkan
{
struct BufferBuilder;
class PipelineBuilder;
class VulkanQueue;

/**
 * @class VulkanDevice
 * @brief Logical Vulkan device with resource creation, command submission, and synchronization
 *
 * Creates the logical device from a physical device, manages queue family setup, and provides
 * resource creation methods (buffers, images, pipelines, etc.). Maintains an immediate command
 * buffer for synchronous GPU operations and a pipeline cache for PSO reuse.
 *
 * Initialization:
 * - Creates queues (graphics, compute, transfer, optionally present)
 * - Initializes immediate command buffer (command pool + buffer + fence)
 * - Creates empty pipeline cache
 * - Enables debug markers (VK_EXT_debug_utils) if available
 */
class VulkanDevice final : public Device
{
public:
    /** @brief Queue types available on the device */
    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
        Present
    };

public:
    /**
     * @brief Creates logical device with specified features
     * @param physical_device The physical device (GPU)
     * @param device_features Feature chain to enable
     */
    VulkanDevice(const VulkanPhysicalDevice& physical_device, const VulkanPhysicalDevice::Features& device_features);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Queue Operations
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Adds present queue for surface presentation
     * @param surface The surface to present to
     */
    void add_present_queue(Surface& surface);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Object Creation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Creates VMA-allocated buffer
     * @param builder Buffer builder with size, usage, and VMA options
     * @return AllocatedBuffer
     */
    [[nodiscard]] AllocatedBuffer create_buffer(const BufferBuilder& builder) const;

    /**
     * @brief Creates VMA-allocated buffer in shared_ptr
     * @param builder Buffer builder
     * @return Shared pointer to AllocatedBuffer
     */
    [[nodiscard]] std::shared_ptr<AllocatedBuffer> create_buffer_shared(const BufferBuilder& builder) const;

    /**
     * @brief Creates VMA-allocated image
     * @param builder Image builder with extent, format, usage, and VMA options
     * @return ImageAllocation
     */
    [[nodiscard]] ImageAllocation create_image(const ImageBuilder& builder) const;

    /**
     * @brief Creates image view
     * @param info Image view create info
     * @return Image view handle
     */
    [[nodiscard]] vk::ImageView create_image_view(const vk::ImageViewCreateInfo& info) const;

    void destory_image_view(vk::ImageView image_view) const;

    /**
     * @brief Creates sampler
     * @param info Sampler create info
     * @return Sampler handle
     */
    [[nodiscard]] vk::raii::Sampler create_sampler(const vk::SamplerCreateInfo& info) const;

    /**
     * @brief Creates descriptor set layout
     * @param builder Descriptor layout builder
     * @return Descriptor set layout handle
     */
    [[nodiscard]] vk::raii::DescriptorSetLayout create_descriptor_set_layout(portal::renderer::vulkan::DescriptorLayoutBuilder& builder) const;

    /**
     * @brief Creates pipeline layout
     * @param pipeline_layout_info Pipeline layout create info
     * @return Pipeline layout handle
     */
    [[nodiscard]] vk::raii::PipelineLayout create_pipeline_layout(const vk::PipelineLayoutCreateInfo& pipeline_layout_info) const;

    /**
     * @brief Creates shader module from SPIR-V bytecode
     * @param code SPIR-V bytecode buffer
     * @return Shader module handle
     */
    [[nodiscard]] vk::raii::ShaderModule create_shader_module(const Buffer& code) const;

    /**
     * @brief Creates graphics or compute pipeline
     * @param builder Pipeline builder with shaders, vertex input, rasterization state, etc.
     * @return Pipeline handle
     */
    [[nodiscard]] vk::raii::Pipeline create_pipeline(vulkan::PipelineBuilder& builder) const;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Command Submission
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Submits commands synchronously (waits for completion)
     * @param function Lambda receiving command buffer for recording
     *
     * Flow:
     * 1. Resets fence and command buffer
     * 2. Begins command buffer
     * 3. Executes function (records commands)
     * 4. Ends command buffer
     * 5. Submits to graphics queue with fence
     * 6. Waits for fence (blocks until completion)
     *
     * Used for one-off operations like initial texture uploads.
     */
    void immediate_submit(std::function<void(const vk::raii::CommandBuffer&)>&& function) const;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Synchronization
    /**
     * Waits for one or more fences to be signaled or until the timeout expires.
     *
     * This method is used to synchronize the execution of the application with
     * the GPU by waiting for specified fences to achieve the signaled state.
     * This can be helpful in ensuring that GPU operations are completed before
     * the application progresses further.
     *
     * @param fenceHandles A list of fence handles to wait on. These fences must
     *        have been created and submitted prior to the invocation of this method.
     * @param timeout The maximum amount of time, in nanoseconds, to wait for the
     *        fences to become signaled. A value of UINT64_MAX can be used to wait
     *        indefinitely.
     * @param waitAll A boolean value indicating whether the function should wait
     *        for all of the specified fences to be signaled (true) or any one
     *        fence to be signaled (false).
     * @return Returns a success or timeout result indicating whether the operation
     *         completed successfully, timed out, or encountered an error.
     */

    void wait_for_fences(vk::ArrayProxy<const vk::Fence> fences, bool wait_all, size_t timeout = std::numeric_limits<size_t>::max()) const;

    /** @brief Waits for all device operations to complete */
    void wait_idle() const;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Getters
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /** @brief Gets device handle (mutable) */
    [[nodiscard]] vk::raii::Device& get_handle() { return device; }

    /** @brief Gets device handle (const) */
    [[nodiscard]] const vk::raii::Device& get_handle() const { return device; }

    /** @brief Gets graphics queue */
    [[nodiscard]] const VulkanQueue& get_graphics_queue() const;

    /** @brief Gets compute queue */
    [[nodiscard]] const VulkanQueue& get_compute_queue() const;

    /** @brief Gets transfer queue */
    [[nodiscard]] const VulkanQueue& get_transfer_queue() const;

    /** @brief Gets present queue */
    [[nodiscard]] const VulkanQueue& get_present_queue() const;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Debug
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Sets debug name using StringId
     * @param handle Vulkan handle
     * @param name Debug name as StringId
     */
    template <typename T>
    void set_debug_name(const T& handle, StringId name) const
    {
        set_debug_name(handle, name.string.data());
    }

    /**
     * @brief Sets debug name for Vulkan object (handles both RAII and plain handles)
     * @param handle Vulkan handle (vk::Buffer or vk::raii::Buffer)
     * @param name Debug name (appears in GPU debuggers)
     */
    template <typename T>
    void set_debug_name(const T& handle, const char* name) const
    {
        using HandleType = std::remove_cv_t<std::remove_reference_t<T>>;

        uint64_t object_handle;
        if constexpr (requires { typename HandleType::CppType; })
        {
            // RAII wrapper type (like vk::raii::Buffer)
            object_handle = VK_HANDLE_CAST(handle);
        }
        else
        {
            // Plain handle type (like vk::Buffer)
            object_handle = reinterpret_cast<uint64_t>(static_cast<typename HandleType::CType>(handle));
        }

        set_debug_name(HandleType::objectType, object_handle, name);
    }

    /**
     * @brief Sets debug name (low-level)
     * @param type Vulkan object type
     * @param handle Handle as uint64_t
     * @param name Debug name
     */
    void set_debug_name(vk::ObjectType type, uint64_t handle, const char* name) const;

private:
    /** @brief Command buffer for synchronous GPU operations */
    struct ImmediateCommandBuffer
    {
        vk::raii::CommandPool command_pool = nullptr;
        vk::raii::CommandBuffer command_buffer = nullptr;
        vk::raii::Fence fence = nullptr;
    };

private:
    void initialize_immediate_commands();

private:
    const VulkanPhysicalDevice& physical_device;
    vk::raii::Device device = nullptr;

    std::unordered_map<QueueType, VulkanQueue> queues;

    ImmediateCommandBuffer immediate_command_buffer;
    vk::raii::PipelineCache pipeline_cache = nullptr;

    bool debug_marker_enabled = false;
};
} // portal
