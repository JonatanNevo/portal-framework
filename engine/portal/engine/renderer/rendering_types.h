//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

#include "vulkan/allocated_buffer.h"

namespace portal::renderer::vulkan
{
/**
 * @struct Vertex
 * @brief GPU vertex buffer layout
 *
 * Interleaved vertex attributes matching shader input layout. Memory layout is optimized
 * for GPU access with vec4 alignment (position+uv_x, normal+uv_y, color).
 */
struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

/**
 * @struct GPUMeshBuffers
 * @brief GPU mesh buffer references
 *
 * GPU-side buffer handles for indexed mesh rendering. Includes device address for
 * buffer device address (BDA) feature enabling shader access without descriptors.
 */
struct GPUMeshBuffers
{
    Buffer index_buffer;
    Buffer vertex_buffer;
    vk::DeviceAddress vertex_buffer_address{};
};

/**
 * @struct GPUCameraData
 * @brief GPU camera uniform buffer layout
 *
 * Camera matrices uploaded to GPU uniform buffer. Includes forward and inverse transforms
 * for view-space, clip-space, and world-space conversions.
 */
struct GPUCameraData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;
    glm::mat4 inverse_view;
    glm::mat4 inverse_proj;
    glm::mat4 inverse_view_proj;
};

/**
 * @struct GPUSceneData
 * @brief GPU scene uniform buffer layout
 *
 * Scene-global data uploaded to GPU. Contains camera transforms and lighting parameters
 * matching shader uniform block layout.
 */
struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;
    glm::vec4 ambient_color;
    glm::vec4 sunlight_direction; ///< w component is sun power
    glm::vec4 sunlight_color;
};

/**
 * @struct GPUDrawPushConstants
 * @brief GPU push constant layout
 *
 * Per-draw data pushed directly to command buffer. Contains model transform and vertex
 * buffer address for bindless rendering.
 */
struct GPUDrawPushConstants
{
    glm::mat4 world_matrix;
    vk::DeviceAddress vertex_buffer{};
};

/**
 * @struct Bounds
 * @brief GPU bounding volume layout
 *
 * Dual bounding volume (sphere + AABB) for GPU-side frustum and occlusion culling.
 */
struct Bounds
{
    glm::vec3 origin{};
    float sphere_radius{};
    glm::vec3 extents{};
};
}
