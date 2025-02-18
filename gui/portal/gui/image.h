//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace portal {

    enum class ImageFormat
    {
        None = 0,
        RGBA,
        RGBA32F
    };

class Image {
public:
    Image(std::string_view path);
    Image(size_t width, size_t height, ImageFormat format, const void* data = nullptr);
    ~Image();

    void set_data(const void* data);

    VkDescriptorSet get_descriptor_set() const { return descriptor_set; }

    void resize(size_t width, size_t height);
    size_t get_width() const { return width; }
    size_t get_height() const { return height; }

    static void* decode(const void* data, size_t length, size_t& out_width, size_t& out_height);
private:
    void allocate_memory(size_t size);
    void release();
private:
    size_t width = 0, height = 0;

    vk::Image image;
    vk::ImageView image_view;
    vk::DeviceMemory memory;
    vk::Sampler sampler;

    ImageFormat format = ImageFormat::None;

    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_memory;

    size_t aligned_size = 0;

    vk::DescriptorSet descriptor_set;
    std::string file_path;

};

} // portal
