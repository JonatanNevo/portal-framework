//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include <set>

#include "portal/application/vulkan/common.h"
#include "portal/application/vulkan/base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;

enum ImageFormat
{
    sRGB,
    UNORM
};

struct SwapchainProperties
{
    vk::SwapchainKHR old_swapchain;
    uint32_t image_count = 3;
    vk::Extent2D extent{};
    vk::SurfaceFormatKHR surface_format{};
    uint32_t array_layers;
    vk::ImageUsageFlags image_usage;
    vk::SurfaceTransformFlagBitsKHR pre_transform;
    vk::CompositeAlphaFlagBitsKHR composite_alpha;
    vk::PresentModeKHR present_mode;
};

class Swapchain final : public VulkanResource<vk::SwapchainKHR>
{
public:
    /**
     * @brief Constructor to create a swapchain by changing the extent
     *        only and preserving the configuration from the old swapchain.
     */
    Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent);

    /**
     * @brief Constructor to create a swapchain by changing the image count
     *        only and preserving the configuration from the old swapchain.
     */
    Swapchain(Swapchain& old_swapchain, uint32_t image_count);

    /**
     * @brief Constructor to create a swapchain by changing the image usage
     *        only and preserving the configuration from the old swapchain.
     */
    Swapchain(Swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags);

    /**
     * @brief Constructor to create a swapchain by changing the extent
     *        and transform only and preserving the configuration from the old swapchain.
     */
    Swapchain(Swapchain& old_swapchain, const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

    /**
     * @brief Constructor to create a swapchain by changing the compression settings
     *        only and preserving the configuration from the old swapchain.
     */
    Swapchain(
        Swapchain& old_swapchain,
        const vk::ImageCompressionFlagsEXT requested_compression,
        const vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate
    );

    /**
     * @brief Constructor to create a swapchain.
     */
    Swapchain(
        Device& device,
        vk::SurfaceKHR surface,
        const vk::PresentModeKHR present_mode,
        const std::vector<vk::PresentModeKHR>& present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
        const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list = {
            {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
            {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
        },
        const vk::Extent2D& extent = {},
        uint32_t image_count = 3,
        vk::SurfaceTransformFlagBitsKHR transform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        const std::set<vk::ImageUsageFlagBits>& image_usage_flags = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
        vk::ImageCompressionFlagsEXT requested_compression = vk::ImageCompressionFlagBitsEXT::eDefault,
        vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate = vk::ImageCompressionFixedRateFlagBitsEXT::eNone
    );

    /**
     * @brief Constructor to create a swapchain from the old swapchain
     *        by configuring all parameters.
     */
    Swapchain(
        Swapchain& old_swapchain,
        Device& device,
        vk::SurfaceKHR surface,
        vk::PresentModeKHR present_mode,
        const std::vector<vk::PresentModeKHR>& present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
        const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list = {
            {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
            {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
        },
        const vk::Extent2D& extent = {},
        uint32_t image_count = 3,
        vk::SurfaceTransformFlagBitsKHR transform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        const std::set<vk::ImageUsageFlagBits>& image_usage_flags = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
        vk::ImageCompressionFlagsEXT requested_compression = vk::ImageCompressionFlagBitsEXT::eDefault,
        vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate = vk::ImageCompressionFixedRateFlagBitsEXT::eNone
    );
    Swapchain(Swapchain&& other) noexcept;
    ~Swapchain() override;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    [[nodiscard]] bool is_valid() const;

    [[nodiscard]] std::pair<vk::Result, uint32_t> acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const;
    [[nodiscard]] const vk::Extent2D& get_extent() const;
    [[nodiscard]] vk::Format get_format() const;
    [[nodiscard]] vk::SurfaceFormatKHR get_surface_format() const;
    [[nodiscard]] const std::vector<vk::Image>& get_images() const;
    [[nodiscard]] vk::SurfaceTransformFlagBitsKHR get_transform() const;
    [[nodiscard]] vk::SurfaceKHR get_surface() const;
    [[nodiscard]] vk::ImageUsageFlags get_usage() const;
    [[nodiscard]] vk::PresentModeKHR get_present_mode() const;

    /**
     * Helper functions for compression controls
     */

    struct SurfaceFormatCompression
    {
        vk::SurfaceFormat2KHR surface_format{};
        vk::ImageCompressionPropertiesEXT compression_properties{};
    };

private:
    vk::SurfaceKHR surface;
    std::vector<vk::Image> images;
    SwapchainProperties properties{};
    // A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
    std::vector<vk::PresentModeKHR> present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox};
    // A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
    std::vector<vk::SurfaceFormatKHR> surface_format_priority_list = {
        {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
        {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
    };
    std::set<vk::ImageUsageFlagBits> image_usage_flags;
    vk::ImageCompressionFlagsEXT requested_compression =vk::ImageCompressionFlagBitsEXT::eDefault;
    vk::ImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate = vk::ImageCompressionFixedRateFlagBitsEXT::eNone;
};
} // portal
