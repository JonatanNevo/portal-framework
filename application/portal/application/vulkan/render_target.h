//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include <functional>

#include "portal/application/vulkan/common.h"
#include "portal/serialization/serialize.h"

namespace portal::vulkan
{
class Image;
class ImageView;
class Device;

/**
 * @brief Description of render pass attachments.
 * Attachment descriptions can be used to automatically create render target images.
 */
struct Attachment
{
    vk::Format format = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
};

/**
 * @brief RenderTarget contains three vectors for: vulkan::Image, vulkan::ImageView and Attachment.
 * The first two are Vulkan images and corresponding image views respectively.
 * Attachment (s) contain a description of the images, which has two main purposes:
 * - RenderPass creation only needs a list of Attachment (s), not the actual images, so we keep
 *   the minimum amount of information necessary
 * - Creation of a RenderTarget becomes simpler, because the caller can just ask for some
 *   Attachment (s) without having to create the images
 */
class RenderTarget
{
public:
    static const std::function<std::unique_ptr<RenderTarget>(Image&&)> DEFAULT_CREATE_FUNC;

    RenderTarget(std::vector<Image>&& images);
    RenderTarget(std::vector<ImageView>&& views);

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&& rt) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget& operator=(RenderTarget&&) = delete;

    const vk::Extent2D& get_extent() const;
    const std::vector<ImageView>& get_views() const;
    const std::vector<Attachment>& get_attachments() const;

    /**
     * @brief Sets the current input attachments overwriting the current ones
     *        Should be set before beginning the render pass and before starting a new subpasses
     * @param input Set of attachment reference number to use as input
     */
    void set_input_attachments(const std::vector<uint32_t>& input);
    const std::vector<uint32_t>& get_input_attachments() const;

    /**
     * @brief Sets the current output attachments overwriting the current ones
     *        Should be set before beginning the render pass and before starting a new subpasses
     * @param output Set of attachment reference number to use as output
     */
    void set_output_attachments(const std::vector<uint32_t>& output);
    const std::vector<uint32_t>& get_output_attachments() const;

    void set_layout(uint32_t attachment, vk::ImageLayout layout);
    vk::ImageLayout get_layout(uint32_t attachment) const;

private:
    Device& device;
    vk::Extent2D extent;
    std::vector<Image> images;
    std::vector<ImageView> views;
    std::vector<Attachment> attachments;
    std::vector<uint32_t> input_attachments = {};   // By default there are no input attachments
    std::vector<uint32_t> output_attachments = {0}; // By default the output attachments is attachment 0
};
} // portal
