//
// Created by thejo on 4/15/2025.
//

#pragma once

#include <unordered_map>
#include <utility>

#include "GLFW/glfw3.h"
#include "portal/core/timer.h"
#include "portal/input/input_types.h"
#include "portal/input/key_state.h"

namespace portal
{

// TODO: add reference to input id, eg keyboard, gamepad, etc
struct InputKeyParams
{
    /** The key that has been pressed */
    Key key = Keys::Invalid;

    /** The event that has caused a Button key to be considered */
    InputEvent event = InputEvent::Pressed;

    /** The number of samples to be taken into account with the KeyState::sample_count_accumulator */
    int32_t num_samples = 0;

    /** The time between the previous frame and the current one */
    float delta_time = 1 / 60.f;

    /** The Delta that the given key/axis has been changed by */
    glm::vec3 delta = glm::vec3(0.f);

    InputKeyParams() = default;

    InputKeyParams(Key key, InputEvent event, glm::vec3 delta) :
        key(std::move(key)), event(event), delta(delta)
    {
    };

    InputKeyParams(Key key, InputEvent event, double delta) :
        key(std::move(key)), event(event), delta({delta, 0, 0})
    {
    };

    InputKeyParams(Key key, float delta_time, int32_t num_samples):
        key(std::move(key)), num_samples(num_samples), delta_time(delta_time)
    {
    };

    InputKeyParams(Key key, glm::vec3 delta, float delta_time, int32_t num_samples):
        key(std::move(key)), delta(delta), num_samples(num_samples), delta_time(delta_time)
    {
    };


};

class Input
{
public:
    explicit Input(GLFWwindow* window);
    bool input_key(const InputKeyParams& params);

    void process_inputs(const float delta_time);

protected:
    void evaluate_key_states(const float delta_time, std::unordered_map<Key, KeyState*>& keys_with_events);
    void finish_processing_inputs();

private:
    GLFWwindow* window;
    std::unordered_map<Key, KeyState> key_state_map;
    uint32_t event_count;

    // Mouse smoothing sample data
    float zero_time[2];    /** How long received mouse movement has been zero. */
    float smoothed_mouse[2];    /** Current average mouse movement/sample */
    int32_t mouse_samples;    /** Number of mouse samples since mouse movement has been zero */
    float mouse_sampling_total;    /** DirectInput's mouse sampling total time */
    Timer timer; // TODO: Have some global timer for all events?
};

} // portal
