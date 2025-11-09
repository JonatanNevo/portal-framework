//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <unordered_map>
#include <utility>

#include "bindings.h"
#include "GLFW/glfw3.h"
#include "portal/core/timer.h"
#include "portal/input/input_types.h"
#include "portal/input/key_state.h"

namespace portal
{

// TODO(#41): add reference to input id, eg keyboard, gamepad, etc
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
        key(std::move(key)), num_samples(num_samples), delta_time(delta_time), delta(delta)
    {
    };


};

class Input
{
public:
    void initialize(GLFWwindow* window_handle);

    bool input_key(const InputKeyParams& params);

    void process_inputs(const float delta_time);

    /**
     * Adds the specified action binding.
     *
     * @param binding The binding to add.
     * @return The last binding in the list.
     * @see clear_action_binding, get_action_binding, get_num_action_bindings, remove_action_binding
     */
    input::ActionBinding& add_action_binding(input::ActionBinding binding);

    /**
     * Removes all action bindings.
     *
     * @see add_action_binding, get_action_binding, get_num_action_bindings, remove_action_binding
     */
    void clear_action_binding();

    /**
     * Gets the action binding with the specified index.
     *
     * @param binding_index The index of the binding to get.
     * @see add_action_binding, clear_action_binding, get_num_action_bindings, remove_action_binding
     */
    [[nodiscard]] input::ActionBinding& get_action_binding(const int32_t binding_index) const { return *action_bindings[binding_index]; }

    /**
     * Gets the number of action bindings.
     *
     * @return Number of bindings.
     * @see add_action_binding, clear_action_binding, get_action_binding, remove_action_binding
     */
    [[nodiscard]] size_t get_num_action_binding() const { return action_bindings.size(); }

    /**
     * Binds a delegate function to an Action defined in the project settings.
     * Returned reference is only guaranteed to be valid until another action is bound.
     */
    template <class T>
    input::ActionBinding& bind_action(
        const Key& key,
        const InputEvent event,
        T* object,
        typename Delegate<void>::NonConstMemberFunction<T> func)
    {
        input::ActionBinding binding(key, event);
        binding.delegate.bind_delegate(object, func);
        return add_action_binding(std::move(binding));
    }

    template <class T>
    input::ActionBinding& bind_action(
        const Key& key,
        const InputEvent event,
        T* object,
        typename Delegate<void, Key>::NonConstMemberFunction<T> func)
    {
        input::ActionBinding binding(key, event);
        binding.delegate.bind_delegate(object, func);
        return add_action_binding(std::move(binding));
    }

protected:
    void evaluate_key_states(float delta_time, std::unordered_map<Key, KeyState*>& keys_with_events);
    void evaluate_input_delegates(float delta_time, std::unordered_map<Key, KeyState*>& keys_with_events);
    void finish_processing_inputs();

private:
    GLFWwindow* window;
    std::unordered_map<Key, KeyState> key_state_map;
    uint32_t event_count;

    // Mouse smoothing sample data
    //float zero_time[2];         /** How long received mouse movement has been zero. */
    float smoothed_mouse[2];    /** Current average mouse movement/sample */
    int32_t mouse_samples;      /** Number of mouse samples since mouse movement has been zero */
    float mouse_sampling_total; /** DirectInput's mouse sampling total time */
    Timer timer;                // TODO: Have some global timer for all events?

    std::vector<std::shared_ptr<input::ActionBinding>> action_bindings;
};

} // portal
