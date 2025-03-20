//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "render_target.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/image.h"
#include "portal/application/vulkan/image_view.h"

namespace portal::vulkan
{
const std::function<std::unique_ptr<RenderTarget>(Image&&)> RenderTarget::DEFAULT_CREATE_FUNC = [](Image&& swapchain_image) -> std::unique_ptr<RenderTarget>
{
    const vk::Format depth_format = get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle());

    ImageBuilder builder(swapchain_image.get_extent());
    builder
        .with_format(depth_format)
        .with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment)
        .with_vma_usage(vma::MemoryUsage::eGpuOnly);

    std::vector<Image> images;
    images.push_back(std::move(swapchain_image));
    images.push_back(builder.build(swapchain_image.get_device()));

    return std::make_unique<RenderTarget>(std::move(images));
};

RenderTarget::RenderTarget(std::vector<Image>&& images): device(images.back().get_device()), images(std::move(images))
{
    auto it = std::ranges::find_if(this->images, [](const auto& image) { return image.get_type() != vk::ImageType::e2D; });
    if (it != this->images.end())
        throw std::runtime_error("Image type is not 2D");

    extent.width = this->images.front().get_extent().width;
    extent.height = this->images.front().get_extent().height;

    // check that every image has the same extent
    it = std::ranges::find_if(
        this->images,
        [this](const auto& image) { return (extent.width != image.get_extent().width) || (extent.height != image.get_extent().height); }
    );
    if (it != this->images.end())
        throw std::runtime_error("Images have different extent");

    for (auto& image : this->images)
    {
        views.emplace_back(image, vk::ImageViewType::e2D);
        attachments.emplace_back(Attachment{image.get_format(), image.get_sample_count(), image.get_usage()});
    }
}

RenderTarget::RenderTarget(std::vector<ImageView>&& views): device(views.back().get_image().get_device()), views(std::move(views))
{
    const uint32_t mip_level = this->views.front().get_subresource_range().baseMipLevel;
    extent.width = this->views.front().get_image().get_extent().width >> mip_level;
    extent.height = this->views.front().get_image().get_extent().height >> mip_level;


    // check that every image view has the same extent
    auto it = std::find_if(
        std::next(this->views.begin()),
        this->views.end(),
        [this](const auto& image_view)
        {
            const uint32_t mip = image_view.get_subresource_range().baseMipLevel;
            return (extent.width != image_view.get_image().get_extent().width >> mip) ||
                (extent.height != image_view.get_image().get_extent().height >> mip);
        }
    );

    if (it != views.end())
        throw std::runtime_error("Image views have different extent");

    for (auto& view : this->views)
    {
        const auto& image = view.get_image();
        attachments.emplace_back(Attachment{image.get_format(), image.get_sample_count(), image.get_usage()});
    }
}

const vk::Extent2D& RenderTarget::get_extent() const
{
    return extent;
}

const std::vector<ImageView>& RenderTarget::get_views() const
{
    return views;
}

const std::vector<Attachment>& RenderTarget::get_attachments() const
{
    return attachments;
}

void RenderTarget::set_input_attachments(const std::vector<uint32_t>& input)
{
    input_attachments = input;
}

const std::vector<uint32_t>& RenderTarget::get_input_attachments() const
{
    return input_attachments;
}

void RenderTarget::set_output_attachments(const std::vector<uint32_t>& output)
{
    output_attachments = output;
}

const std::vector<uint32_t>& RenderTarget::get_output_attachments() const
{
    return output_attachments;
}

void RenderTarget::set_layout(const uint32_t attachment, const vk::ImageLayout layout)
{
    attachments[attachment].initial_layout = layout;
}

vk::ImageLayout RenderTarget::get_layout(const uint32_t attachment) const
{
    return attachments[attachment].initial_layout;
}
} // portal
