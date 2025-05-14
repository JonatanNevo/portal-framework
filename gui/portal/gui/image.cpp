//
// Created by Jonatan Nevo on 31/01/2025.
//

#include "image.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "portal/gui/gui_application.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace portal
{
    namespace utils
    {
        static uint32_t get_vulkan_memory_type(const vk::MemoryPropertyFlags properties, const uint32_t type_bits)
        {
            const auto props = GUIApplication::get_physical_device().getMemoryProperties();
            for (uint32_t i = 0; i < props.memoryTypeCount; i++)
            {
                if ((props.memoryTypes[i].propertyFlags & properties) == properties && (type_bits & (1 << i)))
                    return i;
            }

            return 0xffffffff;
        }

        static uint32_t bytes_per_pixel(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA:
                return 4;
            case ImageFormat::RGBA32F:
                return 16;
            default:
                return 0;
            }
        }

        static vk::Format format_to_vulkan_format(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA:
                return vk::Format::eR8G8B8A8Unorm;
            case ImageFormat::RGBA32F:
                return vk::Format::eR32G32B32A32Sfloat;
            default:
                return vk::Format::eUndefined;
            }
        }

    } // namespace utils

    Image::Image(const std::string_view path) : file_path(path)
    {
        int im_width, im_height, channels;
        uint8_t* data = nullptr;

        if (stbi_is_hdr(file_path.c_str()))
        {
            data = reinterpret_cast<uint8_t*>(stbi_loadf(file_path.c_str(), &im_width, &im_height, &channels, STBI_rgb_alpha));
            format = ImageFormat::RGBA32F;
        }
        else
        {
            data = stbi_load(file_path.c_str(), &im_width, &im_height, &channels, STBI_rgb_alpha);
            format = ImageFormat::RGBA;
        }

        this->width = im_width;
        this->height = im_height;

        allocate_memory(im_width * im_height * utils::bytes_per_pixel(format));
        set_data(data);
        stbi_image_free(data);
    }

    Image::Image(const uint32_t width, const uint32_t height, const ImageFormat format, const void* data) : width(width), height(height), format(format)
    {
        allocate_memory(width * height * utils::bytes_per_pixel(format));
        if (data)
            set_data(data);
    }

    Image::~Image() { release(); }


    void Image::allocate_memory(size_t /*size*/)
    {
        const vk::Device device = GUIApplication::get_device();
        const vk::Format vk_format = utils::format_to_vulkan_format(this->format);

        const vk::ImageCreateInfo image_create_info(
            {},
            vk::ImageType::e2D,
            vk_format,
            vk::Extent3D(width, height, 1),
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
        image = device.createImage(image_create_info);
        const vk::MemoryRequirements req = device.getImageMemoryRequirements(image);
        const vk::MemoryAllocateInfo alloc_info(
            req.size, utils::get_vulkan_memory_type(vk::MemoryPropertyFlagBits::eDeviceLocal, req.memoryTypeBits));
        memory = device.allocateMemory(alloc_info);
        device.bindImageMemory(image, memory, 0);

        constexpr vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        const vk::ImageViewCreateInfo image_view_info({}, image, vk::ImageViewType::e2D, vk_format, {}, subresource_range);
        image_view = device.createImageView(image_view_info);

        vk::SamplerCreateInfo sample_info(
            {},
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat,
            -1000.f,
            true,
            1000.f);
        sampler = device.createSampler(sample_info);

        descriptor_set = ImGui_ImplVulkan_AddTexture(sampler, image_view, static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal));
    }

    void Image::release()
    {
        GUIApplication::submit_resource_free(
            [sampler = sampler,
             image_view = image_view,
             image = image,
             memory = memory,
             staging_buffer = staging_buffer,
             staging_memory = staging_memory]()
            {
                const auto device = GUIApplication::get_device();

                device.destroySampler(sampler);
                device.destroyImageView(image_view);
                device.destroyImage(image);
                device.freeMemory(memory);
                device.destroyBuffer(staging_buffer);
                device.freeMemory(staging_memory);
            });

        sampler = nullptr;
        image_view = nullptr;
        image = nullptr;
        memory = nullptr;
        staging_buffer = nullptr;
        staging_memory = nullptr;
    }

    void Image::set_data(const void* data)
    {
        const auto device = GUIApplication::get_device();

        const size_t upload_size = width * height * utils::bytes_per_pixel(format);

        if (!staging_buffer)
        {
            const vk::BufferCreateInfo info({}, upload_size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
            staging_buffer = device.createBuffer(info);

            auto req = device.getBufferMemoryRequirements(staging_buffer);
            aligned_size = req.size;

            const vk::MemoryAllocateInfo alloc_info(
                req.size, utils::get_vulkan_memory_type(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits));
            staging_memory = device.allocateMemory(alloc_info);
            device.bindBufferMemory(staging_buffer, staging_memory, 0);
        }

        // Upload to buffer
        char* map = nullptr;
        auto result = device.mapMemory(staging_memory, 0, aligned_size, {}, reinterpret_cast<void**>(&map));
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to map memory");
        memcpy(map, data, upload_size);

        vk::MappedMemoryRange range[1] = {};
        range[0].memory = staging_memory;
        range[0].size = aligned_size;
        result = device.flushMappedMemoryRanges(1, range);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to map memory");
        device.unmapMemory(staging_memory);

        // Copy to image
        const auto command_buffer = GUIApplication::get_command_buffer();

        const vk::ImageMemoryBarrier copy_barrier(
            {},
            vk::AccessFlagBits::eTransferWrite,
            {},
            vk::ImageLayout::eTransferDstOptimal,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, copy_barrier);

        vk::BufferImageCopy region{};
        region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
        region.imageExtent = vk::Extent3D{static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1};

        command_buffer.copyBufferToImage(staging_buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

        vk::ImageMemoryBarrier use_barrier = {};
        use_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        use_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        use_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        use_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier.image = image;
        use_barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, use_barrier);

        GUIApplication::flush_command_buffer(command_buffer);
    }

    void Image::resize(uint32_t new_width, uint32_t new_height)
    {
        if (image && new_width == this->width && new_height == this->height)
            return;

        this->width = new_width;
        this->height = new_height;

        release();
        allocate_memory(new_width * new_height * utils::bytes_per_pixel(format));
    }


    void* Image::decode(const void* data, const size_t length, uint32_t& out_width, uint32_t& out_height)
    {
        int temp_width, temp_height, channels;
        uint8_t* temp_data = nullptr;

        temp_data = stbi_load_from_memory(static_cast<const stbi_uc*>(data), static_cast<int>(length), &temp_width, &temp_height, &channels, STBI_rgb_alpha);

        out_width = temp_width;
        out_height = temp_height;
        return temp_data;
    }

} // namespace portal
