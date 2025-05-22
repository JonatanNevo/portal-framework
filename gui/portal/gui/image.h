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
    Image(uint32_t width, uint32_t height, ImageFormat format, const void* data = nullptr);
    ~Image();

    void set_data(const void* data);

    VkDescriptorSet get_descriptor_set() const { return descriptor_set; }

    void resize(uint32_t new_width, uint32_t new_height);
    uint32_t get_width() const { return width; }
    uint32_t get_height() const { return height; }

    static void* decode(const void* data, size_t length, uint32_t& out_width, uint32_t& out_height);
private:
    void allocate_memory(size_t size);
    void release();
private:
    uint32_t width = 0, height = 0;

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
