//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/engine/renderer/vulkan/allocated_buffer.h>

#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/rendering_types.h"
#include "portal/engine/resources/resources/mesh_geometry.h"

namespace portal::renderer {
}

namespace portal::renderer
{
class Material;

struct RenderObject
{
    uint32_t index_count = 0;
    uint32_t first_index = 0;
    std::shared_ptr<vulkan::AllocatedBuffer> index_buffer = nullptr;

    Reference<Material> material = nullptr;
    resources::Bounds bounds{};

    glm::mat4 transform = glm::mat4(1.0f);
    vk::DeviceAddress vertex_buffer_address = 0;

    [[nodiscard]] bool is_visible(const glm::mat4& view_projection) const
    {
        constexpr std::array clip_space_corners{
            glm::vec3{1, 1, 1},
            glm::vec3{1, 1, -1},
            glm::vec3{1, -1, 1},
            glm::vec3{1, -1, -1},
            glm::vec3{-1, 1, 1},
            glm::vec3{-1, 1, -1},
            glm::vec3{-1, -1, 1},
            glm::vec3{-1, -1, -1},
        };

        glm::mat4 matrix = view_projection * transform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            // project each corner into clip space
            glm::vec4 v = matrix * glm::vec4(bounds.origin + (clip_space_corners[c] * bounds.extents), 1.f);

            // perspective correction
            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        // check the clip space box is within the view
        if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
        {
            return false;
        }
        return true;
    }
};

/**
 * @struct FrameResources
 * @brief Per-frame resources for N-frames-in-flight rendering
 *
 * Contains all per-frame GPU resources needed for N-buffered rendering.
 * Each frame has independent command pool/buffer, synchronization primitives, descriptor allocator,
 * and deletion queue to prevent conflicts between in-flight frames.
 *
 * Destructor flushes deletion queue and destroys descriptor pools.
 */
struct FrameResources
{
    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::CommandBuffer command_buffer = nullptr;

    // Semaphore signaled when an image is acquired
    vk::raii::Semaphore image_available_semaphore = nullptr;

    // Fence to signal that command buffers are ready to be reused
    vk::raii::Fence wait_fence = nullptr;

    DeletionQueue deletion_queue = {};

    vk::raii::DescriptorSet global_descriptor_set = nullptr;
    vulkan::AllocatedBuffer scene_data_buffer = nullptr;
    vulkan::DescriptorAllocator frame_descriptors;

    FrameResources(auto&& command_pool, auto&& command_buffer, auto&& image_available_sema, auto&& wait_fence, auto&& descriptors) :
        command_pool(std::forward<decltype(command_pool)>(command_pool)),
        command_buffer(std::forward<decltype(command_buffer)>(command_buffer)),
        image_available_semaphore(std::forward<decltype(image_available_sema)>(image_available_sema)),
        wait_fence(std::forward<decltype(wait_fence)>(wait_fence)),
        frame_descriptors(std::forward<decltype(descriptors)>(descriptors))
    {}

    FrameResources(const FrameResources&) = delete;
    FrameResources& operator=(const FrameResources&) = delete;

    FrameResources(FrameResources&&)  noexcept = default;
    FrameResources& operator=(FrameResources&&) = default;

    ~FrameResources()
    {
        deletion_queue.flush();

        global_descriptor_set = nullptr;
        frame_descriptors.clear_pools();
        frame_descriptors.destroy_pools();
        scene_data_buffer = nullptr;
    }
};

struct FrameDrawImageContext
{
    Reference<Image> draw_image;
    Reference<ImageView> draw_image_view;
    Reference<Image> depth_image;
    Reference<ImageView> depth_image_view;

    size_t last_used_frame_index = 0;
};

/**
 * Per frame rendering context (what to render and where)
 */
struct FrameRenderingContext
{
    // TODO: make this more generic? maybe based on active scene?
    vulkan::GPUSceneData scene_data{};
    vulkan::GPUCameraData camera_data{};
    glm::uvec4 viewport_bounds;

    FrameDrawImageContext image_context{};

    vk::raii::CommandBuffer& command_buffer;
    FrameResources& resources;

    llvm::SmallVector<RenderObject> render_objects;
};
} // portal
