//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/camera.h"

namespace portal
{
enum class CameraMode
{
    Flycam,
    Arcball
};

class EditorCamera final : public ng::Camera
{
public:
    EditorCamera(Input& input, float fov, float width, float height, float near_clip, float far_clip);

    void update(float dt);

    [[nodiscard]] glm::vec3 get_up_direction() const;
    [[nodiscard]] glm::vec3 get_forward_direction() const;
    [[nodiscard]] glm::vec3 get_right_direction() const;
    [[nodiscard]] glm::quat get_orientation() const;

private:
    void disable_mouse();
    void enable_mouse();

    float get_speed() const;

    glm::vec2 pan_speed() const;

    void mouse_pan(const glm::vec2& delta);
    void mouse_rotate(const glm::vec2& delta);
    void mouse_zoom(float delta);

    glm::vec3 calculate_position() const;

private:
    // TODO: is this the correct way of doing this?
    Input& input;

    glm::mat4 view;
    glm::vec3 position, direction, focal_point;

    float vertical_fov = 70.f;
    [[maybe_unused]] float near_clip = 10000.f;
    [[maybe_unused]] float far_clip = 0.1f;

    [[maybe_unused]] float aspect_ratio;
    bool active = false;
    [[maybe_unused]] bool panning = false;
    [[maybe_unused]] bool rotating = false;

    glm::vec2 initial_mouse_position;
    glm::vec3 initial_focal_point;
    glm::vec3 initial_rotation;

    float distance;
    float normal_speed = 5.f;

    float pitch;
    float yaw;
    float pitch_delta;
    float yaw_delta;

    glm::vec3 position_delta;
    glm::vec3 right_direction;

    CameraMode mode = CameraMode::Arcball;

    [[maybe_unused]] float min_focus_distance = 100.f;

    glm::uvec4 viewport_bounds;

    constexpr static float MIN_SPEED = 0.000005;
    constexpr static float MAX_SPEED = 100.f;

};
} // portal
