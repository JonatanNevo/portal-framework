//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "../vulkan/allocated_buffer.h"
#include "portal/engine/renderer/rendering_types.h"

namespace portal
{
struct MaterialInstance;

struct RenderObject
{
    uint32_t index_count = 0;
    uint32_t first_index = 0;
    renderer::vulkan::AllocatedBuffer* index_buffer = nullptr;

    MaterialInstance* material = nullptr;
    vulkan::Bounds bounds{};

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

} // portal
