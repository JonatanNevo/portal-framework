//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

#include "vulkan/allocated_buffer.h"

namespace portal::vulkan
{

struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct GPUMeshBuffers
{
    Buffer index_buffer;
    Buffer vertex_buffer;
    vk::DeviceAddress vertex_buffer_address{};
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;
    glm::vec4 ambient_color;
    glm::vec4 sunlight_direction; // w for sun power
    glm::vec4 sunlight_color;
};

struct GPUDrawPushConstants
{
    glm::mat4 world_matrix;
    vk::DeviceAddress vertex_buffer{};
};

struct Bounds
{
    glm::vec3 origin{};
    float sphere_radius{};
    glm::vec3 extents{};
};

}
