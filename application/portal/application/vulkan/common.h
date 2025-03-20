//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once

#include <map>
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <volk.h>
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
#include <glm/gtx/hash.hpp>

#define VK_FLAGS_NONE 0        // Custom define for better code readability

#define DEFAULT_FENCE_TIMEOUT 100000000000        // Default fence timeout in nanoseconds


namespace portal::vulkan
{

template <class T>
using ShaderStageMap = std::map<vk::ShaderStageFlagBits, T>;

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

struct BufferMemoryBarrier
{
    vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags src_access_mask = {};
    vk::AccessFlags dst_access_mask = {};
};

struct ImageMemoryBarrier
{
    vk::PipelineStageFlags src_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags src_access_mask;
    vk::AccessFlags dst_access_mask;
    vk::ImageLayout old_layout = vk::ImageLayout::eUndefined;
    vk::ImageLayout new_layout = vk::ImageLayout::eUndefined;
    uint32_t old_queue_family = VK_QUEUE_FAMILY_IGNORED;
    uint32_t new_queue_family = VK_QUEUE_FAMILY_IGNORED;
};

struct LoadStoreInfo
{
    vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
};

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a dynamic storage buffer or dynamic uniform buffer.
 * @param descriptor_type Vulkan descriptor type to check.
 * @return True if type is dynamic buffer, false otherwise.
 */
bool is_dynamic_buffer_descriptor_type(vk::DescriptorType descriptor_type);

/**
 * @brief Helper function to determine if a Vulkan descriptor type is a buffer (either uniform or storage buffer, dynamic or not).
 * @param descriptor_type Vulkan descriptor type to check.
 * @return True if type is buffer, false otherwise.
 */
bool is_buffer_descriptor_type(vk::DescriptorType descriptor_type);

/**
 * @brief Helper function to determine a suitable supported depth format based on a priority list
 * @param physical_device The physical device to check the depth formats against
 * @param depth_only (Optional) Whether to include the stencil component in the format or not
 * @param depth_format_priority_list (Optional) The list of depth formats to prefer over one another
 *		  By default we start with the highest precision packed format
 * @return The valid suited depth format
 */
vk::Format get_suitable_depth_format(
    vk::PhysicalDevice physical_device,
    bool depth_only = false,
    const std::vector<vk::Format>& depth_format_priority_list = {
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16Unorm
    }
);

/**
 * @brief Helper function to determine if a Vulkan format is depth only.
 * @param format Vulkan format to check.
 * @return True if format is a depth only, false otherwise.
 */
bool is_depth_only_format(vk::Format format);

/**
 * @brief Helper function to determine if a Vulkan format is depth with stencil.
 * @param format Vulkan format to check.
 * @return True if format is a depth with stencil, false otherwise.
 */
bool is_depth_stencil_format(vk::Format format);

/**
 * @brief Helper function to determine if a Vulkan format is depth.
 * @param format Vulkan format to check.
 * @return True if format is a depth, false otherwise.
 */
bool is_depth_format(vk::Format format);

/**
 * @brief Helper function to combine a given hash
 *        with a generated hash for the input param.
 */
template <class T>
 void hash_combine(size_t &seed, const T &v)
{
    constexpr std::hash<T> hasher;
    glm::detail::hash_combine(seed, hasher(v));
}

template <typename T>
inline std::vector<uint8_t> to_bytes(const T &value)
{
    return std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(&value),
                                reinterpret_cast<const uint8_t *>(&value) + sizeof(T)};
}

}
