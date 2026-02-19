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
 * @struct GPUSceneCamera
 * @brief GPU camera uniform buffer layout
 *
 * Camera matrices uploaded to GPU uniform buffer. Includes forward and inverse transforms
 * for view-space, clip-space, and world-space conversions.
 */
struct GPUSceneCamera
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;
    glm::mat4 inverse_view;
    glm::mat4 inverse_proj;
    glm::mat4 inverse_view_proj;
};

struct GPUScreenData
{
    glm::vec2 full_resolution;
    glm::vec2 half_resolution;
    glm::vec2 inv_full_resolution;
    glm::vec2 inv_half_resolution;
};

struct GPUSceneData
{
    GPUSceneCamera camera;
    GPUScreenData screen_data;
};

struct GPUDirectionalLight
{
    glm::vec3 direction;
    float multiplier;
    glm::vec3 radiance;
    float shadow_amount;
};

struct GPUPointLight
{
    glm::vec3 position;
    float multiplier;
    glm::vec3 radiance;
    float min_radius;
    float radius;
    float falloff;
    float light_size;
    bool casts_shadows;
};

struct GPUSpotLight
{
    glm::vec3 position;
    float multiplier;
    glm::vec3 radiance;
    float angle_attenuation;
    glm::vec3 direction;
    float range;
    float angle;
    float falloff;
    bool soft_shadows;
    bool casts_shadows;
};

struct GPUSceneDirectionLight
{
    GPUDirectionalLight directional_light;
    float environment_map_intensity{};
};

constexpr static unsigned int MAX_POINT_LIGHTS = 128;
constexpr static unsigned int MAX_SPOT_LIGHTS = 128;

struct GPUScenePointLights
{
    unsigned int light_count = 0;
    GPUPointLight point_lights[MAX_POINT_LIGHTS];
};

struct GPUSceneSpotLights
{
    unsigned int light_count = 0;
    GPUSpotLight spot_lights[MAX_SPOT_LIGHTS];
};

struct GPUSceneLights
{
    GPUSceneDirectionLight directional_light;
    GPUScenePointLights point_lights;
    GPUSceneSpotLights spot_lights;
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
