//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <deque>
#include <vulkan/vulkan_raii.hpp>

namespace portal::renderer::vulkan
{
class AllocatedBuffer;
}

namespace portal::vulkan
{

struct DescriptorWriter
{
public:
    void write_image(uint32_t binding, const vk::raii::ImageView& image_view, const vk::raii::Sampler& sampler, vk::ImageLayout layout, vk::DescriptorType type);
    void write_buffer(uint32_t binding, renderer::vulkan::AllocatedBuffer& buffer, size_t size, size_t offset, vk::DescriptorType type);

    void clear();
    void update_set(const vk::raii::Device& device, const vk::raii::DescriptorSet& set);

    std::deque<vk::DescriptorImageInfo> image_infos;
    std::deque<vk::DescriptorBufferInfo> buffer_infos;
    std::vector<vk::WriteDescriptorSet> writes;
};

}
